"""
Кросс-проверка совместимости C++-порта и Python-версии (туда и обратно).

Проверяются:
  * одни и те же параметры (p, q, T) и sha256(T);
  * C++-подпись принимается Python-верификатором и наоборот;
  * C++-конверт (encrypt-then-sign) расшифровывается Python-версией и наоборот;
  * ключи, сгенерированные C++, читаются Python-версией (Y == T^x).

C++-бинарник: ~/bignum/matrix_crypto/matrix_crypto (WSL, Ubuntu-24.04).
Запуск:  python cpp_interop_test.py
"""

import base64
import hashlib
import json
import os
import subprocess
import sys

import matrix_crypto_core as core
import cipher_tool as tool


DISTRO = "Ubuntu-24.04"
BIN = "/home/novikov/bignum/matrix_crypto/build/matrix_crypto"
INTEROP_WSL = "/home/novikov/bignum/matrix_crypto/interop"
INTEROP_WIN = r"\\wsl.localhost\Ubuntu-24.04\home\novikov\bignum\matrix_crypto\interop"

OK = 0
FAIL = 0


def check(ok, name, fail="ПРОВАЛ"):
    global OK, FAIL
    if ok:
        OK += 1
        print(f"    [+] {name}")
    else:
        FAIL += 1
        print(f"    [!] {name}  --  {fail}")


def run_cpp(args, cwd=INTEROP_WSL):
    """Запускает C++-бинарник в WSL с заданным рабочим каталогом."""
    script = "cd " + cwd + " && " + " ".join(args)
    r = subprocess.run(
        ["wsl.exe", "-d", DISTRO, "--", "bash", "-lc", script],
        capture_output=True,
    )
    return r


def b64s(s: str) -> str:
    return base64.b64encode(s.encode("utf-8")).decode("ascii")


def main():
    print("=" * 74)
    print(" КРОСС-ПРОВЕРКА C++ <-> PYTHON (матричная криптосистема v2)")
    print("=" * 74)

    os.makedirs(INTEROP_WIN, exist_ok=True)

    # --- Python генерирует ключи и кладёт их туда, где их прочитает C++ ---
    pub, priv = tool_keygen()
    Y = pub["Y"]
    x = priv["x"]

    with open(os.path.join(INTEROP_WIN, "public_key.json"), "w", encoding="utf-8") as f:
        json.dump(pub, f, indent=4)
    with open(os.path.join(INTEROP_WIN, "private_key.json"), "w", encoding="utf-8") as f:
        json.dump(priv, f, indent=4)

    print("\n--- [1] C++ читает Python-ключи и печатает параметры ---")
    r = run_cpp([BIN, "params"])
    out = r.stdout.decode("utf-8", "replace")
    check("p_bits = 1023" in out, "C++: p_bits = 1023")
    check("q_bits = 256" in out, "C++: q_bits = 256")
    check("base_T_sha256 = " + core.sha256_hex(core.serialize_matrix(core.BASE_T)) in out,
          "C++ и Python согласны по sha256(T)")

    messages = [
        "Привет из Python!",
        "cross-language interop test 12345",
        "Юникод: αβγ δεζ — строка",
    ]

    print("\n--- [2] C++ подписывает -> Python проверяет ---")
    for msg in messages:
        r = run_cpp([BIN, "sign-b64", b64s(msg)])
        sig = r.stdout.decode("ascii").strip()
        check(bool(sig) and tool.verify_with_public(Y, msg, sig),
              f"C++-подпись принята Python для {msg!r}")
        check(not tool.verify_with_public(Y, msg + "!", sig),
              f"Python отвергает C++-подпись для изменённого {msg!r}")

    print("\n--- [3] C++ шифрует (encrypt-then-sign) -> Python расшифровывает ---")
    for msg in messages:
        r = run_cpp([BIN, "seal-b64", b64s(msg)])
        sealed = r.stdout.decode("ascii").strip()
        opened = tool.open_with_keys(x, Y, sealed)
        check(opened == msg, f"C++-конверт расшифрован Python для {msg!r}")

    print("\n--- [4] Python подписывает -> C++ проверяет ---")
    for msg in messages:
        sig = tool.sign_with_key(x, msg)
        out = run_cpp([BIN, "verify-b64", b64s(msg), sig]).stdout.decode("utf-8", "replace")
        check("ВЕРНА" in out, f"C++ принял Python-подпись для {msg!r}")

        out_bad = run_cpp([BIN, "verify-b64", b64s(msg + "!"), sig]).stdout.decode("utf-8", "replace")
        check("ОШИБКА" in out_bad, f"C++ отверг Python-подпись для изменённого {msg!r}")

    print("\n--- [5] Python шифрует (encrypt-then-sign) -> C++ расшифровывает ---")
    for msg in messages:
        sealed = tool.seal_with_keys(x, Y, msg)
        r = run_cpp([BIN, "open", sealed])
        check(r.stdout.decode("utf-8", "replace") == msg,
              f"C++ расшифровал Python-конверт для {msg!r}")

    print("\n--- [6] C++ генерирует ключи -> Python проверяет Y == T^x ---")
    sub = INTEROP_WSL + "/cppkeys"
    os.makedirs(INTEROP_WIN + r"\cppkeys", exist_ok=True)
    r = run_cpp([BIN, "keygen", "public_key.json", "private_key.json"], cwd=sub)
    with open(os.path.join(INTEROP_WIN, "cppkeys", "public_key.json"), encoding="utf-8") as f:
        cpub = json.load(f)
    with open(os.path.join(INTEROP_WIN, "cppkeys", "private_key.json"), encoding="utf-8") as f:
        cpriv = json.load(f)
    check(cpub["Y"] == core.mat_pow(core.BASE_T, cpriv["x"]),
          "Python подтверждает Y = T^x для C++-ключей")
    check(cpub["base_T_sha256"] == core.sha256_hex(core.serialize_matrix(core.BASE_T)),
          "C++-ключ содержит корректный base_T_sha256")

    # C++-ключом шифруем из Python и расшифровываем в C++
    cY = cpub["Y"]
    cx = cpriv["x"]
    msg = "ключи, созданные C++, используются Python"
    sealed = tool.seal_with_keys(cx, cY, msg)
    r = run_cpp([BIN, "open", sealed], cwd=sub)
    check(r.stdout.decode("utf-8", "replace") == msg,
          "Python зашифровал C++-ключом, C++ расшифровал")

    print("\n--- [7] Аутентифицированный DH: Python(Alice) <-> C++(Bob) ---")
    import keygen

    dh_win = os.path.join(INTEROP_WIN, "dh")
    dh_wsl = INTEROP_WSL + "/dh"
    os.makedirs(dh_win, exist_ok=True)

    # Alice (Python)
    Ay, Ax = keygen.generate_keypair()
    alice_Y, alice_x = Ay["Y"], Ax["x"]
    a = core.random_scalar()
    A = core.mat_pow(core.BASE_T, a)
    packet_a = base64.b64encode(core.serialize_matrix(A)).decode()
    sig_a = tool.sign_with_key(alice_x, packet_a)
    with open(os.path.join(dh_win, "alice_pub.json"), "w", encoding="utf-8") as f:
        json.dump(Ay, f, indent=4)

    # Bob (C++): ключи + подписанный пакет для выбранного b
    run_cpp([BIN, "keygen", "bob_pub.json", "bob_priv.json"], cwd=dh_wsl)
    with open(os.path.join(dh_win, "bob_pub.json"), encoding="utf-8") as f:
        bob_Y = json.load(f)["Y"]
    b = core.random_scalar()
    pub_proc = run_cpp([BIN, "dh-publish", "bob_priv.json", str(b)], cwd=dh_wsl)
    out = pub_proc.stdout.decode("utf-8", "replace")
    packet_b = next((l.split(" ", 1)[1] for l in out.splitlines() if l.startswith("PACKET ")), None)
    sig_b = next((l.split(" ", 1)[1] for l in out.splitlines() if l.startswith("SIG ")), None)
    if packet_b is None:
        check(False, "C++ dh-publish вернул пакет",
              "stdout=" + out[:120] + " stderr=" + pub_proc.stderr.decode("utf-8", "replace")[:120])
        packet_b = sig_b = "AA"

    # Python принимает подпись Боба и вычисляет общий секрет
    check(tool.verify_with_public(bob_Y, packet_b, sig_b), "Python принял подпись пакета Боба (C++)")
    B_mat = core.deserialize_matrix(base64.b64decode(packet_b))
    shared_a = core.mat_pow(B_mat, a)
    alice_key = hashlib.sha256(core.serialize_matrix(shared_a)).hexdigest()

    # C++ принимает подпись Алисы и вычисляет общий секрет
    out = run_cpp([BIN, "dh-derive", str(b), "alice_pub.json", packet_a, sig_a], cwd=dh_wsl)
    otext = out.stdout.decode("utf-8", "replace")
    shared_hex = next((l.split(" ", 1)[1] for l in otext.splitlines() if l.startswith("SHARED_HEX ")), "")
    session = next((l.split(" ", 1)[1] for l in otext.splitlines() if l.startswith("SESSION_KEY ")), "")
    check(shared_hex == core.serialize_matrix(shared_a).hex(),
          "C++ и Python согласны по общему секрету T^(ab)")
    check(session == alice_key, "сессионные ключи SHA-256 совпадают (C++ == Python)")

    # C++ отвергает подделанную подпись пакета пира
    bad_sig = ("A" if sig_a[0] != "A" else "B") + sig_a[1:]
    r_bad = run_cpp([BIN, "dh-derive", str(b), "alice_pub.json", packet_a, bad_sig], cwd=dh_wsl)
    check(r_bad.returncode != 0, "C++ отверг подделанную подпись пакета пира (MitM)")

    print("\n" + "=" * 74)
    if FAIL == 0:
        print(f" INTEROP SUCCESS: пройдено {OK} проверок")
    else:
        print(f" INTEROP FAILED: провалено {FAIL} из {OK + FAIL}")
    print("=" * 74)
    return 0 if FAIL == 0 else 1


def tool_keygen():
    import keygen
    return keygen.generate_keypair()


if __name__ == "__main__":
    sys.exit(main())
