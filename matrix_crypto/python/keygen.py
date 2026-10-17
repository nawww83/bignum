"""
ГЕНЕРАЦИЯ КЛЮЧЕВОЙ ПАРЫ (схема v2).

Секрет: x <- [1, q-1]
Публичный ключ: Y = T^x  (T — генератор подгруппы простого порядка q)

Стойкость опирается на задачу дискретного логарифмирования в <T>.
Восстановить x из Y без перебора невозможно (generic: ~2^128 операций при q ~ 2^256).

Файлы:
  public_key.json  — {algorithm, version, p_bits, q_bits, base_T_sha256, Y}
  private_key.json — {x}
"""

import json
import matrix_crypto_core as core


def generate_keypair():
    """Возвращает (public_dict, private_dict)."""
    x = core.random_scalar()               # секретная экспонента в [1, q-1]
    Y = core.mat_pow(core.BASE_T, x)       # публичный ключ Y = T^x

    public = {
        "algorithm": "Matrix-ElGamal-SL3 (ElGamal + Schnorr над <T> в SL_3(F_p))",
        "version": 2,
        "p_bits": core.P_MODULO.bit_length(),
        "q_bits": core.ORDER_Q.bit_length(),
        "params_file": "matrix_params.py",
        "base_T_sha256": core.sha256_hex(core.serialize_matrix(core.BASE_T)),
        "Y": Y,
    }
    private = {"x": x}
    return public, private


def save_keypair(public, private):
    with open("public_key.json", "w", encoding="utf-8") as f:
        json.dump(public, f, indent=4)
    with open("private_key.json", "w", encoding="utf-8") as f:
        json.dump(private, f, indent=4)


def generate_and_save_keys():
    print("Генерация ключевой пары матричной криптосистемы v2 (ElGamal + Schnorr)...")
    print(f"    Порядок подгруппы q: ~2^{core.ORDER_Q.bit_length()} (generic-стойкость DLP ~2^{core.ORDER_Q.bit_length() // 2})")
    public, private = generate_keypair()
    save_keypair(public, private)
    print(f"    Секретный показатель x: ~{private['x'].bit_length()} бит")
    print("Ключи успешно сохранены в 'public_key.json' и 'private_key.json'")


if __name__ == "__main__":
    generate_and_save_keys()
