"""
CLI-ИНСТРУМЕНТ МАТРИЧНОЙ КРИПТОСИСТЕМЫ v2.

Режимы: encrypt | decrypt | sign | verify

БАЗОВЫЕ ПРИМИТИВЫ
-----------------
ШИФРОВАНИЕ (Эль-Гамаль в подгруппе <T> простого порядка q):
    P             — блок сообщения (матрица 3x3 из элементов F_p)
    r  <- [1,q-1] — одноразовый секрет
    C1 = T^r,  C2 = P * Y^r
    Дешифрование: P = C2 * (C1^x)^(-1)      <- ТРЕБУЕТСЯ СЕКРЕТ x

ПОДПИСЬ (схема Шнорра в <T>):
    k <- [1,q-1],  R = T^k,  e = H(R || message) mod q,  s = k + e*x mod q
    Проверка: T^s == R * Y^e                <- подделка требует x

АУТЕНТИФИЦИРОВАННЫЙ ФОРМАТ (encrypt-then-sign)
----------------------------------------------
Сам по себе Эль-Гамаль НЕ аутентифицирует шифртекст: он мультипликативно
изменяем — C2' = Z*C2 расшифровывается в Z*P, то есть атакующий без ключа
может управляемо менять открытый текст, и получатель этого не заметит.

Поэтому CLI по умолчанию использует конверт: сначала ШИФРУЕМ, затем
ПОДПИСЫВАЕМ полученный шифртекст ключом отправителя. Получатель сначала
проверяет подпись и только затем расшифровывает.

    sealed = base64( {"v": 2, "ct": <base64 шифртекста>, "sig": <подпись(ct)>} )

Сырые примитивы (encrypt_with_public / decrypt_with_private) оставлены для
низкоуровневых сценариев и тестов атак.
"""

import sys
import json
import base64

import matrix_crypto_core as core


# ---------------------------------------------------------------------------
# Ключи
# ---------------------------------------------------------------------------

def load_public_key():
    with open("public_key.json", "r", encoding="utf-8") as f:
        d = json.load(f)
    return d["Y"]


def load_private_key():
    with open("private_key.json", "r", encoding="utf-8") as f:
        d = json.load(f)
    return d["x"]


# ---------------------------------------------------------------------------
# БАЗОВЫЙ ПРИМИТИВ: ШИФРОВАНИЕ / ДЕШИФРОВАНИЕ (без аутентификации)
# ---------------------------------------------------------------------------

def encrypt_with_public(Y, text: str) -> str:
    blocks = core.encode_message(text)
    ints = []
    for P in blocks:
        r = core.random_scalar()                    # одноразовый секрет
        C1 = core.mat_pow(core.BASE_T, r)           # T^r
        Yr = core.mat_pow(Y, r)                     # Y^r (без x из C1 не вычислить)
        C2 = core.mat_mul(P, Yr)                    # P * Y^r
        ints.extend(core.flatten_matrix(C1))
        ints.extend(core.flatten_matrix(C2))
    return base64.b64encode(core.pack_int_list(ints)).decode("utf-8")


def decrypt_with_private(x: int, base64_str: str) -> str:
    raw = base64.b64decode(base64_str)
    ints = core.unpack_int_list(raw)
    if len(ints) == 0 or len(ints) % 18 != 0:
        raise ValueError("Некорректная длина шифртекста")

    blocks = []
    for i in range(0, len(ints), 18):
        C1 = core.unflatten_matrix(ints[i:i + 9])
        C2 = core.unflatten_matrix(ints[i + 9:i + 18])

        # Гигиена: C1 обязан лежать в подгруппе <T> (защита от малых подгрупп).
        if core.mat_pow(C1, core.ORDER_Q) != core.IDENTITY:
            raise ValueError("C1 не принадлежит подгруппе <T> (повреждённый шифртекст)")

        S = core.mat_pow(C1, x)                     # S = T^(r*x) = Y^r
        P = core.mat_mul(C2, core.mat_inv(S))       # P = C2 * (Y^r)^(-1)
        blocks.append(P)

    return core.decode_message(blocks)


# ---------------------------------------------------------------------------
# БАЗОВЫЙ ПРИМИТИВ: ПОДПИСЬ / ПРОВЕРКА (схема Шнорра)
# ---------------------------------------------------------------------------

def sign_with_key(x: int, text: str) -> str:
    k = core.random_scalar()                        # одноразовый нонс
    R = core.mat_pow(core.BASE_T, k)                # R = T^k
    e = core.hash_to_scalar(core.serialize_matrix(R), text.encode("utf-8"))
    s = (k + e * x) % core.ORDER_Q                  # s = k + e*x mod q
    payload = core.serialize_matrix(R) + s.to_bytes(core.ELEMENT_BYTES, "big")
    return base64.b64encode(payload).decode("utf-8")


def verify_with_public(Y, text: str, base64_sign: str) -> bool:
    try:
        raw = base64.b64decode(base64_sign)
    except Exception:
        return False
    expected = 9 * core.ELEMENT_BYTES + core.ELEMENT_BYTES
    if len(raw) != expected:
        return False

    R = core.deserialize_matrix(raw[:9 * core.ELEMENT_BYTES])
    s = int.from_bytes(raw[9 * core.ELEMENT_BYTES:], "big")
    if s >= core.ORDER_Q:
        return False
    if core.mat_pow(R, core.ORDER_Q) != core.IDENTITY:
        return False

    e = core.hash_to_scalar(core.serialize_matrix(R), text.encode("utf-8"))
    lhs = core.mat_pow(core.BASE_T, s)              # T^s
    rhs = core.mat_mul(R, core.mat_pow(Y, e))       # R * Y^e
    return lhs == rhs


# ---------------------------------------------------------------------------
# АУТЕНТИФИЦИРОВАННЫЙ ФОРМАТ: encrypt-then-sign
# ---------------------------------------------------------------------------

def seal_with_keys(sender_x: int, recipient_Y, text: str) -> str:
    """Шифрует текст для recipient_Y и подписывает шифртекст ключом sender_x."""
    ct = encrypt_with_public(recipient_Y, text)
    sig = sign_with_key(sender_x, ct)               # подпись ИМЕННО шифртекста
    payload = {"v": 2, "ct": ct, "sig": sig}
    return base64.b64encode(
        json.dumps(payload, separators=(",", ":")).encode("utf-8")
    ).decode("utf-8")


def open_with_keys(recipient_x: int, sender_Y, sealed: str) -> str:
    """Проверяет подпись шифртекста ключом sender_Y и затем расшифровывает."""
    try:
        payload = json.loads(base64.b64decode(sealed).decode("utf-8"))
        ct = payload["ct"]
        sig = payload["sig"]
        if not isinstance(ct, str) or not isinstance(sig, str):
            raise ValueError
    except Exception:
        raise ValueError("Некорректный формат аутентифицированного шифртекста")

    if not verify_with_public(sender_Y, ct, sig):
        raise ValueError("Аутентификация не пройдена: шифртекст изменён или подпись неверна")

    return decrypt_with_private(recipient_x, ct)


# ---------------------------------------------------------------------------
# ОБЁРТКИ НА ГЛОБАЛЬНОЙ ПАРЕ КЛЮЧЕЙ (для CLI)
# ---------------------------------------------------------------------------

def encrypt_action(text: str) -> str:
    """CLI-шифрование в аутентифицированном формате (encrypt-then-sign)."""
    return seal_with_keys(load_private_key(), load_public_key(), text)


def decrypt_action(sealed: str) -> str:
    """CLI-дешифрование: сначала проверяем аутентичность, потом расшифровываем."""
    return open_with_keys(load_private_key(), load_public_key(), sealed)


def sign_action(text: str) -> str:
    return sign_with_key(load_private_key(), text)


def verify_action(text: str, base64_sign: str) -> bool:
    return verify_with_public(load_public_key(), text, base64_sign)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _usage():
    print("Использование:")
    print("  python cipher_tool.py encrypt \"текст\"     (encrypt-then-sign)")
    print("  python cipher_tool.py decrypt \"BASE64\"")
    print("  python cipher_tool.py sign \"текст\"")
    print("  python cipher_tool.py verify \"текст\" \"BASE64_ПОДПИСЬ\"")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        _usage()
        sys.exit(1)

    mode = sys.argv[1].lower()
    try:
        if mode == "encrypt":
            print(encrypt_action(sys.argv[2]))
        elif mode == "decrypt":
            print(decrypt_action(sys.argv[2]))
        elif mode == "sign":
            print(sign_action(sys.argv[2]))
        elif mode == "verify":
            if len(sys.argv) < 4:
                print("Ошибка: для верификации передайте текст и подпись.")
                sys.exit(1)
            ok = verify_action(sys.argv[2], sys.argv[3])
            print(f"\nРезультат проверки подписи: {'ВЕРНА (Успех)' if ok else 'ОШИБКА (Текст изменён, подпись или ключ неверны)'}")
        else:
            print("Неизвестный режим.")
            _usage()
            sys.exit(1)
    except Exception as e:
        print(f"Ошибка: {e}")
        sys.exit(2)
