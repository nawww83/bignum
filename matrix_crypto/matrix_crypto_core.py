"""
МАТЕМАТИЧЕСКОЕ ЯДРО МАТРИЧНОЙ КРИПТОСИСТЕМЫ v2.

Система работает в циклической подгруппе <T> ПРОСТОГО порядка q группы
SL_3(F_p). Параметры (p, q, T) сгенерированы офлайн (matrix_gen.py) и
загружаются из matrix_params.py.

Обозначения:
  p = P_MODULO        — 1024-битное простое (характеристика поля)
  q = GROUP_ORDER_Q   — 256-битное простое (порядок <T>)
  T = BASE_T          — генератор подгруппы, order(T) = q

Все экспоненты (секреты, нонсы) выбираются в [1, q-1], поэтому их разрядность
равна разрядности q (~256 бит), а не «~2^128 порядка группы».
"""

import secrets
import hashlib

from matrix_params import P_MODULO, GROUP_ORDER_Q, BASE_T

MODULO = P_MODULO          # обратная совместимость с прежним именем
ORDER_Q = GROUP_ORDER_Q    # порядок подгруппы <T>
BIT_WIDTH = P_MODULO.bit_length()

# Ширина одного элемента при канонической сериализации (байт)
ELEMENT_BYTES = (BIT_WIDTH + 7) // 8

IDENTITY = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]


# ---------------------------------------------------------------------------
# Матричная арифметика над F_p
# ---------------------------------------------------------------------------

def mat_mul(A, B, mod=MODULO):
    C = [[0] * 3 for _ in range(3)]
    for i in range(3):
        Ai = A[i]
        Ci = C[i]
        for k in range(3):
            aik = Ai[k]
            if aik == 0:
                continue
            Bk = B[k]
            for j in range(3):
                Ci[j] = (Ci[j] + aik * Bk[j]) % mod
    return C


def mat_vec_mul(A, v, mod=MODULO):
    return [sum(A[i][j] * v[j] for j in range(3)) % mod for i in range(3)]


def mat_pow(A, n, mod=MODULO):
    R = [row[:] for row in IDENTITY]
    B = [row[:] for row in A]
    while n > 0:
        if n & 1:
            R = mat_mul(R, B, mod)
        B = mat_mul(B, B, mod)
        n >>= 1
    return R


def mat_det(A, mod=MODULO):
    return (
        A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1])
        - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0])
        + A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0])
    ) % mod


def ext_gcd(a, b):
    if a == 0:
        return b, 0, 1
    g, x1, y1 = ext_gcd(b % a, a)
    return g, y1 - (b // a) * x1, x1


def mod_inverse(a, m):
    g, x, y = ext_gcd(a, m)
    if g != 1:
        raise ValueError("Обратного элемента не существует")
    return x % m


def mat_inv(A, mod=MODULO):
    """
    Обратная матрица для матриц из SL_3(F_p) (det = 1) — через адъюгату,
    без деления на детерминант. Для матриц с det != 1 возбуждается ValueError.
    """
    det = mat_det(A, mod)
    if det != 1:
        raise ValueError(f"mat_inv ожидает матрицу с det=1, получен det={det}")

    inv_mat = [[0] * 3 for _ in range(3)]
    inv_mat[0][0] = (A[1][1] * A[2][2] - A[1][2] * A[2][1]) % mod
    inv_mat[0][1] = (A[0][2] * A[2][1] - A[0][1] * A[2][2]) % mod
    inv_mat[0][2] = (A[0][1] * A[1][2] - A[0][2] * A[1][1]) % mod

    inv_mat[1][0] = (A[1][2] * A[2][0] - A[1][0] * A[2][2]) % mod
    inv_mat[1][1] = (A[0][0] * A[2][2] - A[0][2] * A[2][0]) % mod
    inv_mat[1][2] = (A[0][2] * A[1][0] - A[0][0] * A[1][2]) % mod

    inv_mat[2][0] = (A[1][0] * A[2][1] - A[1][1] * A[2][0]) % mod
    inv_mat[2][1] = (A[0][1] * A[2][0] - A[0][0] * A[2][1]) % mod
    inv_mat[2][2] = (A[0][0] * A[1][1] - A[0][1] * A[1][0]) % mod
    return inv_mat


def random_scalar(mod=ORDER_Q):
    """Секретная экспонента в [1, q-1] (разрядность ~ 256 бит)."""
    return secrets.randbelow(mod - 1) + 1


# ---------------------------------------------------------------------------
# Сериализация и хэширование
# ---------------------------------------------------------------------------

def serialize_matrix(M) -> bytes:
    """Каноническая сериализация 3x3-матрицы: 9 элементов по ELEMENT_BYTES байт."""
    out = bytearray()
    for row in M:
        for x in row:
            out += x.to_bytes(ELEMENT_BYTES, byteorder='big')
    return bytes(out)


def deserialize_matrix(data: bytes):
    if len(data) != 9 * ELEMENT_BYTES:
        raise ValueError("Неверная длина сериализованной матрицы")
    vals = [int.from_bytes(data[i * ELEMENT_BYTES:(i + 1) * ELEMENT_BYTES], 'big')
            for i in range(9)]
    return [vals[0:3], vals[3:6], vals[6:9]]


def flatten_matrix(M):
    return [x for row in M for x in row]


def unflatten_matrix(vals):
    return [list(vals[0:3]), list(vals[3:6]), list(vals[6:9])]


def pack_int_list(ints) -> bytes:
    return b"".join(x.to_bytes(ELEMENT_BYTES, byteorder='big') for x in ints)


def unpack_int_list(data: bytes):
    if len(data) % ELEMENT_BYTES != 0:
        raise ValueError("Длина потока не кратна размеру элемента")
    return [int.from_bytes(data[i:i + ELEMENT_BYTES], 'big')
            for i in range(0, len(data), ELEMENT_BYTES)]


def hash_to_scalar(*chunks: bytes) -> int:
    """Хэш (SHA-512) произвольных данных, приведённый по модулю q."""
    h = hashlib.sha512()
    for c in chunks:
        h.update(c)
    return int.from_bytes(h.digest(), byteorder='big') % ORDER_Q


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


# ---------------------------------------------------------------------------
# Кодирование текста в матричные блоки (по 9 элементов поля на блок)
# ---------------------------------------------------------------------------

def _bytes_to_digits(data: bytes):
    """Байты -> цифры в системе счисления с основанием p (младшая цифра первая)."""
    m = int.from_bytes(data, byteorder='big')
    if m == 0:
        return [0]
    digits = []
    while m > 0:
        digits.append(m % MODULO)
        m //= MODULO
    return digits


def _digits_to_int(digits) -> int:
    m = 0
    for d in reversed(digits):
        m = m * MODULO + d
    return m


def encode_message(text: str):
    """
    Текст -> список блоков (каждый блок — матрица 3x3 из элементов F_p).
    Первый элемент потока — длина исходных байт (для точного восстановления).
    """
    data = text.encode('utf-8')
    digits = _bytes_to_digits(data)
    arr = [len(data)] + digits
    while len(arr) % 9 != 0:
        arr.append(0)
    return [unflatten_matrix(arr[i:i + 9]) for i in range(0, len(arr), 9)]


def decode_message(blocks) -> str:
    arr = []
    for b in blocks:
        arr.extend(flatten_matrix(b))
    length = arr[0]
    m = _digits_to_int(arr[1:])
    if length == 0:
        return ""
    # Санитарная проверка заголовка: длина не может превышать объём данных блока.
    if length > len(arr[1:]) * ELEMENT_BYTES:
        raise ValueError("Повреждённый блок: недопустимая длина сообщения")
    try:
        data = m.to_bytes(length, byteorder='big')
    except OverflowError:
        raise ValueError("Повреждённый блок: данные не соответствуют заявленной длине")
    return data.decode('utf-8')
