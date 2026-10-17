"""
Офлайн-генератор системных параметров матричной криптосистемы v2.

Строится циклическая подгруппа ПРОСТОГО порядка q в SL_3(F_p):

    q  — 256-битное простое, q = 1 (mod 3)
    w  — примитивный кубический корень из единицы по модулю q (w^2 + w + 1 = 0)
    p  — большое простое, p = w (mod q)  =>  q | p^2 + p + 1
    T  — матрица 3x3 над F_p порядка ровно q (генератор подгруппы <T>)

Идея: если p = w (mod q), то p^2 + p + 1 = w^2 + w + 1 = 0 (mod q).
Возьмём сопровождающую матрицу C неприводимого кубического многочлена
f(x) = x^3 - c2 x^2 - c1 x - 1. Такой C лежит в SL_3(F_p), а его порядок делит
p^2 + p + 1 (норма-1 часть мультипликативной группы F_{p^3}).
Тогда T = C^((p^2+p+1)/q) имеет порядок, делящий простое q, то есть ровно q.

Запуск:  python matrix_gen.py
Вывод:   файл matrix_params.py с константами P_MODULO, GROUP_ORDER_Q, BASE_T.
"""

import secrets

# ---------------------------------------------------------------------------
# Простые числа
# ---------------------------------------------------------------------------

_SMALL_PRIMES = [
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67,
    71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149,
    151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
    233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313,
    317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
    419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499,
]


def is_prime(n: int, rounds: int = 40) -> bool:
    """Вероятностный тест Миллера-Рабина (тестовая вероятность ошибки < 2^-80)."""
    if n < 2:
        return False
    for sp in _SMALL_PRIMES:
        if n % sp == 0:
            return n == sp
    d = n - 1
    r = 0
    while d % 2 == 0:
        d //= 2
        r += 1
    for _ in range(rounds):
        a = secrets.randbelow(n - 3) + 2
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(r - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


def gen_prime_q(bits: int = 256) -> int:
    """Простое q разрядности `bits` с условием q = 1 (mod 3)."""
    while True:
        q = secrets.randbits(bits) | (1 << (bits - 1)) | 1
        if q % 3 != 1:
            continue
        if is_prime(q):
            return q


def cube_root_of_unity(q: int) -> int:
    """Примитивный кубический корень из единицы по модулю q (q = 1 mod 3)."""
    assert (q - 1) % 3 == 0
    while True:
        h = secrets.randbelow(q - 3) + 2
        w = pow(h, (q - 1) // 3, q)
        if w != 1:
            # w^3 = 1 и w != 1  =>  порядок ровно 3  =>  w^2 + w + 1 = 0
            assert (w * w + w + 1) % q == 0
            return w


def gen_prime_p(q: int, w: int, p_bits: int = 1024) -> int:
    """Простое p ~ 2^p_bits вида p = w + k*q (тогда q | p^2 + p + 1)."""
    k_bits = p_bits - q.bit_length()
    k = secrets.randbits(k_bits) | (1 << (k_bits - 1))
    while True:
        p = w + k * q
        sieve = True
        for sp in _SMALL_PRIMES:
            if p % sp == 0:
                sieve = False
                break
        if sieve and is_prime(p):
            return p
        k += 1


# ---------------------------------------------------------------------------
# Полиномы над F_p (степени <= 3)
# ---------------------------------------------------------------------------

def _poly_mulmod(a, b, c2, c1, mod):
    """(a * b) mod (x^3 - c2 x^2 - c1 x - 1). Коэффициенты: a = [a0, a1, a2]."""
    conv = [0] * 5
    for i in range(3):
        if a[i] == 0:
            continue
        ai = a[i]
        for j in range(3):
            conv[i + j] = (conv[i + j] + ai * b[j]) % mod
    c4 = conv[4]
    if c4:
        # x^4 = x*x^3 = (c2^2 + c1) x^2 + (c2*c1 + 1) x + c2
        conv[2] = (conv[2] + c4 * (c2 * c2 + c1)) % mod
        conv[1] = (conv[1] + c4 * (c2 * c1 + 1)) % mod
        conv[0] = (conv[0] + c4 * c2) % mod
    c3 = conv[3]
    if c3:
        # x^3 = c2 x^2 + c1 x + 1
        conv[2] = (conv[2] + c3 * c2) % mod
        conv[1] = (conv[1] + c3 * c1) % mod
        conv[0] = (conv[0] + c3) % mod
    return [conv[0] % mod, conv[1] % mod, conv[2] % mod]


def _poly_x_pow_p(c2, c1, mod):
    """x^p mod f, где f(x) = x^3 - c2 x^2 - c1 x - 1."""
    res = [1, 0, 0]
    base = [0, 1, 0]
    e = mod
    while e:
        if e & 1:
            res = _poly_mulmod(res, base, c2, c1, mod)
        base = _poly_mulmod(base, base, c2, c1, mod)
        e >>= 1
    return res


def _poly_trim(a):
    a = a[:]
    while len(a) > 1 and a[-1] == 0:
        a.pop()
    return a


def _poly_mod(a, b, mod):
    a = _poly_trim(a)
    b = _poly_trim(b)
    db = len(b) - 1
    inv_lb = pow(b[-1], mod - 2, mod)
    while len(a) - 1 >= db and not (len(a) == 1 and a[0] == 0):
        shift = len(a) - 1 - db
        coef = a[-1] * inv_lb % mod
        for i in range(db + 1):
            a[shift + i] = (a[shift + i] - coef * b[i]) % mod
        a = _poly_trim(a)
        if len(a) - 1 < db:
            break
    return a


def _poly_gcd(a, b, mod):
    a = _poly_trim(a)
    b = _poly_trim(b)
    while not (len(b) == 1 and b[0] == 0):
        a, b = b, _poly_mod(a, b, mod)
    inv = pow(a[-1], mod - 2, mod)
    return [(c * inv) % mod for c in a]


def is_irreducible_cubic(c2: int, c1: int, mod: int) -> bool:
    """
    Неприводимость f(x) = x^3 - c2 x^2 - c1 x - 1 над F_p.
    Для многочлена степени 3 достаточно: f не имеет корней в F_p, т.е.
    gcd(x^p - x, f) = 1.
    """
    h = _poly_x_pow_p(c2, c1, mod)          # x^p mod f
    gx = [h[0] % mod, (h[1] - 1) % mod, h[2] % mod]  # x^p - x
    f = [(-1) % mod, (-c1) % mod, (-c2) % mod, 1]    # x^3 - c2 x^2 - c1 x - 1
    return _poly_gcd(f, gx, mod) == [1]


# ---------------------------------------------------------------------------
# Матричные операции (нужны для проверки порядка T)
# ---------------------------------------------------------------------------

def _mat_mul(A, B, mod):
    return [[sum(A[i][k] * B[k][j] for k in range(3)) % mod for j in range(3)] for i in range(3)]


def _mat_pow(A, n, mod):
    R = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    B = [row[:] for row in A]
    while n:
        if n & 1:
            R = _mat_mul(R, B, mod)
        B = _mat_mul(B, B, mod)
        n >>= 1
    return R


def _mat_det(A, mod):
    return (
        A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1])
        - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0])
        + A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0])
    ) % mod


IDENTITY = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]


def gen_generator_T(p: int, q: int):
    """Матрица T порядка ровно q в SL_3(F_p)."""
    order_pm1 = p * p + p + 1
    assert order_pm1 % q == 0, "q должно делить p^2 + p + 1"
    m = order_pm1 // q
    while True:
        c2 = secrets.randbelow(p)
        c1 = secrets.randbelow(p)
        if not is_irreducible_cubic(c2, c1, p):
            continue
        C = [[0, 1, 0], [0, 0, 1], [1 % p, c1 % p, c2 % p]]
        assert _mat_det(C, p) == 1
        T = _mat_pow(C, m, p)
        if T == IDENTITY:
            continue
        # Проверки-инварианты (должны выполняться всегда):
        if _mat_pow(T, q, p) != IDENTITY:
            continue
        if _mat_det(T, p) != 1:
            continue
        return T


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    print("=" * 70)
    print(" ГЕНЕРАЦИЯ СИСТЕМНЫХ ПАРАМЕТРОВ МАТРИЧНОЙ КРИПТОСИСТЕМЫ v2")
    print("=" * 70)

    print("\n[1/4] Генерация простого q (256 бит, q = 1 mod 3)...")
    q = gen_prime_q(256)
    print(f"      q = {q}")
    print(f"      q бит: {q.bit_length()}")

    print("\n[2/4] Кубический корень из единицы w по модулю q...")
    w = cube_root_of_unity(q)
    print(f"      w = {w}")
    assert (w * w + w + 1) % q == 0

    print("\n[3/4] Генерация простого p ~ 2^1024, p = w (mod q)...")
    p = gen_prime_p(q, w, 1024)
    print(f"      p бит: {p.bit_length()}")
    assert p % q == w
    assert (p * p + p + 1) % q == 0
    print(f"      проверка: q | p^2 + p + 1  ->  OK")

    print("\n[4/4] Поиск генератора T порядка q (неприводимый кубический многочлен)...")
    T = gen_generator_T(p, q)
    print("      T найден, проверка порядка...")
    assert _mat_pow(T, q, p) == IDENTITY
    assert T != IDENTITY
    print("      T^q = I и T != I  ->  order(T) = q  ->  OK")

    with open("matrix_params.py", "w", encoding="utf-8") as f:
        f.write('"""\n')
        f.write('СИСТЕМНЫЕ ПАРАМЕТРЫ МАТРИЧНОЙ КРИПТОСИСТЕМЫ v2.\n\n')
        f.write('ФАЙЛ СГЕНЕРИРОВАН АВТОМАТИЧЕСКИ (matrix_gen.py). Не редактировать вручную.\n\n')
        f.write('Подгруппа <T> в SL_3(F_p) имеет ПРОСТОЙ порядок q (~2^256).\n')
        f.write('Стойкость дискретного логарифмирования в <T>: ~2^128 (generic, rho/BSGS).\n')
        f.write('p ~ 2^1024 выбран так, чтобы index calculus в F_{p^3} (~2^3072) не был проще.\n')
        f.write('"""\n\n')
        f.write(f"P_MODULO = {p}\n\n")
        f.write(f"GROUP_ORDER_Q = {q}\n\n")
        f.write("BASE_T = [\n")
        for row in T:
            f.write(f"    [{row[0]}, {row[1]}, {row[2]}],\n")
        f.write("]\n")

    print("\n[+] Параметры записаны в 'matrix_params.py'")
    print("=" * 70)


if __name__ == "__main__":
    main()
