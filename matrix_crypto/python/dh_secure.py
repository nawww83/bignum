"""
ИМИТАЦИЯ СЕТЕВОГО ОБМЕНА КЛЮЧАМИ (аутентифицированный матричный Диффи-Хеллман).

Схема (v2):
  * Диффи-Хеллман в подгруппе <T> простого порядка q:
        A = T^a,  B = T^b,  общий секрет = T^(ab)
  * Каждый пакет подписывается схемой Шнорра (требует секрета x),
    поэтому подделать пакет без закрытого ключа невозможно.

Отличие от прежней версии: подписи действительно нельзя подделать по открытому
ключу, а согласование ключа устойчиво к пассивному перехвату (DLP в <T>).
"""

import sys
import hashlib
import base64

import matrix_crypto_core as core
import cipher_tool as tool
import keygen


def pack_matrix_to_base64(M) -> str:
    return base64.b64encode(core.serialize_matrix(M)).decode("utf-8")


def unpack_matrix_from_base64(b64_str: str):
    return core.deserialize_matrix(base64.b64decode(b64_str))


if __name__ == "__main__":
    print("=" * 74)
    print(" АУТЕНТИФИЦИРОВАННЫЙ ОБМЕН КЛЮЧАМИ (МАТРИЧНЫЙ DH + ПОДПИСЬ ШНОРРА)")
    print("=" * 74)

    # --- ключевые пары сторон (эфемерные, не сохраняются на диск) ---
    alice_pub, alice_priv = keygen.generate_keypair()
    bob_pub, bob_priv = keygen.generate_keypair()
    alice_Y, alice_x = alice_pub["Y"], alice_priv["x"]
    bob_Y, bob_x = bob_pub["Y"], bob_priv["x"]

    print("\n--- [ЭТАП 1: АЛИСА ФОРМИРУЕТ И ПОДПИСЫВАЕТ ПАКЕТ] ---")
    alice_dh_private = core.random_scalar()
    alice_pub_matrix = core.mat_pow(core.BASE_T, alice_dh_private)     # A = T^a
    alice_packet = pack_matrix_to_base64(alice_pub_matrix)
    alice_signature = tool.sign_with_key(alice_x, alice_packet)        # подпись Шнорра
    print(f"Пакет Алисы (Base64): {alice_packet[:40]}...")
    print(f"Подпись Алисы       : {alice_signature[:40]}...")

    print("\n--- [ЭТАП 2: БОБ ПРОВЕРЯЕТ ПОДПИСЬ АЛИСЫ] ---")
    if not tool.verify_with_public(alice_Y, alice_packet, alice_signature):
        print("КРИТИЧЕСКАЯ ОШИБКА: подпись Алисы неверна! Возможна MitM-атака.")
        sys.exit(1)
    print("[+] Успех: подпись Алисы подтверждена. Пакет подлинный.")
    received_alice_matrix = unpack_matrix_from_base64(alice_packet)

    print("\n--- [ЭТАП 3: БОБ ФОРМИРУЕТ И ПОДПИСЫВАЕТ СВОЙ ПАКЕТ] ---")
    bob_dh_private = core.random_scalar()
    bob_pub_matrix = core.mat_pow(core.BASE_T, bob_dh_private)         # B = T^b
    bob_packet = pack_matrix_to_base64(bob_pub_matrix)
    bob_signature = tool.sign_with_key(bob_x, bob_packet)
    print(f"Пакет Боба (Base64) : {bob_packet[:40]}...")
    print(f"Подпись Боба        : {bob_signature[:40]}...")

    print("\n--- [ЭТАП 4: АЛИСА ПРОВЕРЯЕТ ПОДПИСЬ БОБА] ---")
    if not tool.verify_with_public(bob_Y, bob_packet, bob_signature):
        print("КРИТИЧЕСКАЯ ОШИБКА: подпись Боба неверна! Канал скомпрометирован.")
        sys.exit(1)
    print("[+] Успех: подпись Боба подтверждена. Пакет подлинный.")
    received_bob_matrix = unpack_matrix_from_base64(bob_packet)

    print("\n--- [ЭТАП 5: НЕЗАВИСИМОЕ ВЫЧИСЛЕНИЕ ОБЩЕГО СЕКРЕТА] ---")
    alice_shared = core.mat_pow(received_bob_matrix, alice_dh_private)  # B^a = T^(ab)
    bob_shared = core.mat_pow(received_alice_matrix, bob_dh_private)    # A^b = T^(ab)

    alice_key = hashlib.sha256(core.serialize_matrix(alice_shared)).hexdigest()
    bob_key = hashlib.sha256(core.serialize_matrix(bob_shared)).hexdigest()

    print(f"Сессионный ключ Алисы: {alice_key}")
    print(f"Сессионный ключ Боба : {bob_key}")
    ok = (alice_shared == bob_shared) and (alice_key == bob_key)
    print(f"\nОбщий секретный ключ сформирован? {'ДА (Успех)' if ok else 'НЕТ (Ошибка)'}")
    sys.exit(0 if ok else 1)
