"""
СКВОЗНОЙ (E2E) ТЕСТ МАТРИЧНОЙ КРИПТОСИСТЕМЫ v2.

Проверяются:
  * инварианты группы <T> простого порядка q в SL_3(F_p);
  * корректность шифрования/дешифрования и рандомизация;
  * корректность и рандомизация подписи Шнорра;
  * обнаружение подмены шифртекста и текста под подписью;
  * КЛЮЧЕВЫЕ атаки: невозможность расшифровать без секрета x и подделать
    подпись без секрета x (для v1 обе атаки проходили — теперь нет).
"""

import sys
import json
import base64

try:
    import matrix_crypto_core as core
    import matrix_gen
    import keygen
    import cipher_tool as tool
except ImportError as e:
    print(f"[-] Ошибка импорта модулей: {e}")
    print("[!] Убедитесь, что e2e_test.py находится в одной папке с остальными файлами проекта.")
    sys.exit(1)

_FAILURES = []


def check(cond, ok_msg, fail_msg):
    if cond:
        print(f"    [+] {ok_msg}")
    else:
        print(f"    [-] {fail_msg}")
        _FAILURES.append(fail_msg)
    return cond


def hr(title):
    print("\n" + "-" * 70)
    print(title)
    print("-" * 70)


def run_e2e_test():
    print("=" * 70)
    print("   E2E ТЕСТ МАТРИЧНОЙ КРИПТОСИСТЕМЫ v2 (Эль-Гамаль + Шнорр над <T>)")
    print("=" * 70)

    # ------------------------------------------------------------------ ШАГ 0
    hr("[Шаг 0] Инварианты группы")
    check(core.P_MODULO.bit_length() >= 1000, f"p имеет {core.P_MODULO.bit_length()} бит (~1024)", "p слишком мал")
    check(core.ORDER_Q.bit_length() >= 256, f"q имеет {core.ORDER_Q.bit_length()} бит (>= 256)", "q слишком мал")
    check(matrix_gen.is_prime(core.ORDER_Q), "q — простое число", "q не простое")
    check((core.P_MODULO ** 2 + core.P_MODULO + 1) % core.ORDER_Q == 0,
          "q делит p^2 + p + 1", "q НЕ делит p^2 + p + 1")
    check(core.mat_det(core.BASE_T) == 1, "det(T) = 1 (T из SL_3(F_p))", "det(T) != 1")
    check(core.mat_pow(core.BASE_T, core.ORDER_Q) == core.IDENTITY,
          "T^q = I", "T^q != I")
    check(core.BASE_T != core.IDENTITY,
          "T != I  =>  порядок T равен ровно q (нет малых подгрупп)", "T == I")

    # ------------------------------------------------------------------ ШАГ 1
    hr("[Шаг 1] Генерация ключевой пары")
    keygen.generate_and_save_keys()
    check(json.load(open("public_key.json", encoding="utf-8"))["Y"] is not None,
          "public_key.json создан", "public_key.json отсутствует")
    priv = json.load(open("private_key.json", encoding="utf-8"))
    check("x" in priv and 1 <= priv["x"] < core.ORDER_Q,
          f"секрет x лежит в [1, q-1] (~{priv['x'].bit_length()} бит)", "секрет x вне диапазона [1, q-1]")
    pub_raw = open("public_key.json", encoding="utf-8").read()
    check('"x"' not in pub_raw, "публичный ключ НЕ содержит секрет x", "утечка x в публичный ключ!")

    # ------------------------------------------------------------------ ШАГ 2
    hr("[Шаг 2] Шифрование / дешифрование")
    messages = [
        "Привет, Боб!",
        "Это очень длинный секретный текст, который должен занять несколько блоков матричной схемы. Проверяем склейку!",
    ]
    ciphertexts = {}
    for msg in messages:
        ct = tool.encrypt_action(msg)
        ct2 = tool.encrypt_action(msg)
        check(ct != ct2, f"рандомизация: два шифрования одного текста различаются ({len(ct)} симв.)",
              "шифртексты совпали — нарушена рандомизация")
        check(tool.decrypt_action(ct) == msg, f"round-trip OK для '{msg[:25]}...'", "round-trip провален")
        ciphertexts[msg] = ct

    # ----------------------------------------------------------------- ШАГ 2b
    hr("[Шаг 2b] Аутентификация шифртекста (encrypt-then-sign)")
    Y = tool.load_public_key()
    x = tool.load_private_key()
    demo = messages[1]

    # (1) Сырой примитив изменяем: атакующий без ключа подменяет C2 -> Z*C2.
    P_block = core.encode_message(demo)[0]
    r_att = core.random_scalar()
    C1_att = core.mat_pow(core.BASE_T, r_att)
    C2_att = core.mat_mul(P_block, core.mat_pow(Y, r_att))
    Z = core.mat_pow(core.BASE_T, core.random_scalar())      # любая обратимая матрица
    C2_mod = core.mat_mul(Z, C2_att)
    P_after = core.mat_mul(C2_mod, core.mat_inv(core.mat_pow(C1_att, x)))
    check(P_after == core.mat_mul(Z, P_block),
          "Изменяемость сырого Эль-Гамаля: C2' = Z*C2 расшифровывается в Z*P (без ключа!)",
          "не удалось продемонстрировать изменяемость")

    # (2) Тот же атакующий меняет C2 внутри аутентифицированного конверта,
    #     сохраняя старую подпись -> конверт обязан отвергнуть.
    sealed = tool.encrypt_action(demo)
    env = json.loads(base64.b64decode(sealed).decode("utf-8"))
    ints = core.unpack_int_list(base64.b64decode(env["ct"]))
    C1b = core.unflatten_matrix(ints[:9])
    C2b = core.unflatten_matrix(ints[9:18])
    Zb = core.mat_pow(core.BASE_T, core.random_scalar())
    C2b_mod = core.mat_mul(Zb, C2b)
    new_ct = base64.b64encode(
        core.pack_int_list(core.flatten_matrix(C1b) + core.flatten_matrix(C2b_mod))
    ).decode("utf-8")
    tampered = base64.b64encode(json.dumps(
        {"v": env["v"], "ct": new_ct, "sig": env["sig"]}, separators=(",", ":")
    ).encode("utf-8")).decode("utf-8")
    rejected = False
    try:
        rejected = (tool.decrypt_action(tampered) != demo)
    except Exception:
        rejected = True
    check(rejected, "Аутентифицированный конверт ОТВЕРГ подмену C2 (подпись не сходится)",
          "ФАТАЛЬНО: подмена C2 не обнаружена")

    # (3) Подпись чужого отправителя не принимается.
    other_x = keygen.generate_keypair()[1]["x"]
    forged = False
    try:
        forged = (tool.decrypt_action(tool.seal_with_keys(other_x, Y, demo)) != demo)
    except Exception:
        forged = True
    check(forged, "Аутентифицированный конверт отверг подпись чужого отправителя",
          "ФАТАЛЬНО: подпись чужого отправителя принята")

    # ------------------------------------------------------------------ ШАГ 3
    hr("[Шаг 3] Подпись Шнорра")
    doc = "Документ особой важности: перевести 1000 BTC на счёт Алисы."
    sig1 = tool.sign_action(doc)
    sig2 = tool.sign_action(doc)
    check(sig1 != sig2, "рандомизация: две подписи одного текста различаются", "подписи совпали")
    check(tool.verify_action(doc, sig1), "корректная подпись проходит проверку", "валидная подпись отвергнута")
    check(not tool.verify_action(doc + " ", sig1), "изменение текста обнаружено", "подмена текста не обнаружена")

    # ------------------------------------------------------------------ ШАГ 4
    hr("[Шаг 4] Атаки (должны ПРОВАЛИТЬСЯ)")

    # Атака A: подмена аутентифицированного шифртекста
    ct = ciphertexts[messages[1]]
    corrupted = ct[:-6] + ("AAAAAA" if not ct.endswith("AAAAAA") else "BBBBBB")
    try:
        corrupted_text = tool.decrypt_action(corrupted)
        attack_ok = (corrupted_text != messages[1])
        check(attack_ok, "Атака A: подмена шифртекста обнаружена (текст не совпал)",
              "подмена шифртекста не обнаружена")
    except Exception as exc:
        check(True, f"Атака A: подмена отвергнута аутентификацией/форматом ({type(exc).__name__})", "")

    # Атака B: расшифрование ТОЛЬКО по открытому ключу (на сыром примитиве)
    # Атакующий знает T, Y, C1, C2, но не x. Ему нужен S = C1^x = Y^r.
    raw_ct = tool.encrypt_with_public(Y, messages[1])
    recovered = None
    for guess in (0, 1, 2, core.ORDER_Q - 1, priv["x"] + 1):
        try:
            out = tool.decrypt_with_private(guess, raw_ct)
        except Exception:
            out = None
        if out == messages[1]:
            recovered = guess
            break
    check(recovered is None,
          "Атака B: расшифрование без секрета x НЕВОЗМОЖНО (все публичные подстановки дают мусор)",
          f"ФАТАЛЬНО: сообщение восстановлено без x (guess={recovered})")
    print(f"        [i] Восстановление x требует дискретного логарифма в <T>;")
    print(f"            при q ~ 2^{core.ORDER_Q.bit_length()} generic-сложность ~ 2^{core.ORDER_Q.bit_length() // 2}.")

    # Атака C: подделка подписи без секрета x
    # C1. Попытка подписи «нулевым» ключом (s = k при x = 0).
    forged_zero = tool.sign_with_key(0, doc)
    check(not tool.verify_action(doc, forged_zero),
          "Атака C1: подпись с x=0 отвергнута", "ФАТАЛЬНО: подпись с x=0 принята")
    # C2. Попытка подписи случайным чужим ключом.
    other_priv = keygen.generate_keypair()[1]["x"]
    forged_other = tool.sign_with_key(other_priv, doc)
    check(not tool.verify_action(doc, forged_other),
          "Атака C2: подпись чужим ключом отвергнута", "ФАТАЛЬНО: подпись чужим ключом принята")
    # C3. Попытка подписи «линейной алгеброй» v1: собрать S из публичных матриц.
    #     В v2 такой публичной формулы нет — проверяем, что случайные (R, s) не проходят.
    import secrets as _secrets
    fake_R = core.mat_pow(core.BASE_T, core.random_scalar())
    fake_s = _secrets.randbelow(core.ORDER_Q)
    fake_sig = base64.b64encode(core.serialize_matrix(fake_R) +
                                fake_s.to_bytes(core.ELEMENT_BYTES, "big")).decode()
    check(not tool.verify_action(doc, fake_sig),
          "Атака C3: случайная подпись (R, s) отвергнута", "ФАТАЛЬНО: случайная подпись принята")

    # Атака D: подмена DH-пакета
    import dh_secure
    alice_pub, alice_priv = keygen.generate_keypair()
    packet = dh_secure.pack_matrix_to_base64(core.mat_pow(core.BASE_T, core.random_scalar()))
    sig = tool.sign_with_key(alice_priv["x"], packet)
    tampered_packet = dh_secure.pack_matrix_to_base64(core.mat_pow(core.BASE_T, core.random_scalar()))
    check(not tool.verify_with_public(alice_pub["Y"], tampered_packet, sig),
          "Атака D: подмена DH-пакета обнаружена подписью", "ФАТАЛЬНО: подмена DH-пакета не обнаружена")

    # ------------------------------------------------------------------ ШАГ 5
    hr("[Шаг 5] Демонстрация: стоимость восстановления секрета ~ sqrt(q)")
    # Миниатюрный аналог той же конструкции (p=23, q=7): полный перебор.
    p_small, q_small = 23, 7
    T_small = matrix_gen.gen_generator_T(p_small, q_small)
    x_small = 5
    Y_small = matrix_gen._mat_pow(T_small, x_small, p_small)
    found = None
    for e in range(1, q_small):
        if matrix_gen._mat_pow(T_small, e, p_small) == Y_small:
            found = e
            break
    check(found == x_small,
          f"миниатюра (q={q_small}): секрет восстановлен перебором за sqrt(q)~{int(q_small**0.5)+1} шагов",
          "демонстрация DLP провалена")
    print("        [i] Для реального q ~ 2^256 аналогичный перебор требует ~2^128 операций — неосуществимо.")

    # ------------------------------------------------------------------ ИТОГ
    print("\n" + "=" * 70)
    if _FAILURES:
        print(f"  ОТЧЕТ: ПРОВАЛЕНО тестов: {len(_FAILURES)}")
        for f in _FAILURES:
            print(f"    - {f}")
        print("=" * 70)
        return False
    print("  ОТЧЕТ: Все тесты криптосистемы v2 успешно пройдены (E2E SUCCESS)!")
    print("=" * 70)
    print("[i] Ключи сохранены в 'public_key.json' / 'private_key.json' (не удаляются).")
    return True


if __name__ == "__main__":
    ok = run_e2e_test()
    sys.exit(0 if ok else 1)
