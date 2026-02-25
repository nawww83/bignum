#pragma once

#include "u128.hpp"
#include "i128.hpp"
#include <cassert>
#include <utility> // std::pair
#include "ubig.hpp"
#include "ulow.hpp"

namespace u128_utils
{

using namespace bignum::u128;

/**
 * @brief Целочисленный квадратный корень sqrt(x).
 * @param exact Возвращает true, если x — полный квадрат.
 */
inline U128 isqrt(const U128& x, bool& exact)
{
    if (x == 0) {
        exact = true;
        return 0;
    }

    // Начальное приближение: 2^(ceil(bits/2))
    U128 x0 = U128{1} << ((x.bit_width() + 1) / 2);
    U128 x1;

    for (;;) {
        U128 remainder;
        // ОПТИМИЗАЦИЯ: Получаем частное и остаток за один проход
        U128 quotient = U128::divide<true, true>(x, x0, &remainder);
        
        x1 = (x0 + quotient) >> 1;

        // В целочисленном методе Ньютона x1 >= x0 означает сходимость к floor(sqrt)
        if (x1 >= x0) {
            exact = (x0 * x0 == x);
            return x0;
        }
        x0 = x1;
    }
}

// Перегрузка для удобства
inline U128 isqrt(const U128& x) {
    bool dummy;
    return isqrt(x, dummy);
}

/**
 * @brief Целочисленная степень числа.
 * @details Предполагается, что переполнения не будет, т.е. расчет идет по модулю 2^128.
 * @param x
 * @param y
 * @return
 */
inline U128 int_power(const U128& x, unsigned y)
{
    U128 result{1};
    for (unsigned i = 1; i <= y; ++i)
        result = result * x;
    return result;
}

/**
 * @brief Быстрое возведение в степень x^y mod 2^128.
 * Использует алгоритм бинарного возведения в квадрат.
 */
inline U128 int_power_fast(U128 x, uint32_t y) noexcept
{
    if (y == 0) return U128{1};
    if (x == 0) return U128{0};
    if (x == 1) return U128{1};
    if (x == 2) return (y < 128) ? (U128{1} << y) : U128{0};

    U128 result{1};
    for (;;) {
        if (y & 1) result *= x;
        y >>= 1;
        if (y == 0) break; 
        x *= x;
    }
    return result;
}


/**
 * @brief Целочисленный корень m-й степени из x.
 */
inline U128 nroot(const U128& x, uint32_t m)
{
    if (m == 0) return 0;
    if (x <= 1 || m == 1) return x;
    if (m >= 128) return (x > 0) ? U128{1} : U128{0};
    if (m == 2) return isqrt(x);

    // 1. Начальное приближение "сверху"
    // Берем 2^(ceil(bit_width / m))
    uint32_t target_bits = (x.bit_width() + m - 1) / m;
    U128 x0 = U128{1} << target_bits;
    
    // Страховка: если 2^target_bits оказался больше x
    if (x0 > x) x0 = x;

    U128 m_val{m};
    U128 m_minus_1 = m_val - 1;

    for (;;) {
        U128 p = int_power_fast(x0, m - 1);
        U128 quotient = (p == 0) ? U128{0} : (x / p);

        // Стандартная формула Ньютона: x1 = ((m-1)*x0 + quotient) / m
        // Чтобы избежать переполнения (m-1)*x0, считаем через разность:
        U128 x1;
        if (x0 > quotient) {
            // Идем вниз: x1 = x0 - (x0 - quotient) / m
            U128 diff = (x0 - quotient) / m_val;
            
            // КРИТИЧЕСКИЙ МОМЕНТ: если diff == 0, но x0 > quotient, 
            // это не значит, что мы на месте. Это значит, что шаг < 1.
            // В целых числах нам нужно принудительно сделать шаг -1.
            if (diff == 0) x1 = x0 - 1;
            else x1 = x0 - diff;
        } else {
            // Если x0 <= quotient, мы либо нашли корень, либо зашли снизу.
            // Метод Ньютона сверху вниз гарантирует, что x0 >= floor(root).
            return x0;
        }

        // Если x1 перелетел через корень (стал слишком маленьким)
        // или если мы начали расти — останавливаемся.
        if (x1 >= x0) return x0;

        // Проверка: не стал ли x1^m меньше x? 
        // Если стал — значит x1 и есть наш floor(root).
        // Но проще довериться сходимости и сделать еще одну итерацию.
        x0 = x1;
    }
}

/**
 * @brief Является ли число x квадратичным вычетом по модулю p.
 */
inline bool is_quadratic_residue(const U128& x, const U128& p)
{
    assert(p != 0);
    const auto rx = x % p;
    U128 y2 = 0;
    for (U128 y = 0; y < p; y++)
    {                
        if (const auto ry2 = y2 % p; ry2 == rx)
            return true;
        y2 += (y + y + 1);
    }
    return false; 
}

/**
 * @brief Возвращает величину, обратную x по модулю m.
 * @param a Входное число.
 * @param m Модуль.
 * @param success Флаг успешности нахождения обратной величины.
 * @return Обратная к x величина, y, так, что y*x = 1 mod m.
 */
inline U128 modular_inverse(U128 a, U128 m, bool &success)
{
    using namespace bignum::i128;
    const I128 m0 = m;
    I128 y = 0;
    I128 x = 1;
    success = false;
    if (m == 1)
        return 0;
    while (a > 1)
    {
        if (m == 0)
            return U128{};
        const I128 q = a / m;
        I128 temp = m;
        m = a % m;
        a = temp.unsigned_part();
        temp = y;
        y = x - q * y;
        x = temp;
    }
    x += x < 0 ? m0 : 0;
    success = true;
    return x.unsigned_part();
}

/**
 * @brief Квадратный корень числа x по модулю p.
 */
inline std::pair<U128, U128> sqrt_mod(const U128& x, const U128& p)
{
    assert(p != 0);
    U128 result[2];
    int idx = 0;
    const auto rx = x % p;
    U128 y2 = 0;
    for (U128 y = 0; y < p; y++)
    {
        if (const auto ry2 = y2 % p; ry2 == rx)
            result[idx++] = y;
        y2 += (y + y + 1);
        if (idx == 2) break;
    }
    if (idx == 1)
        result[1] = result[0];
    return std::make_pair(result[0], result[1]);
}

/**
 * @brief Деление числа x на y по модулю p. По определению без остатка.
 */
inline U128 div_mod(const U128& x, const U128& y, const U128& p)
{
    assert(p != 0);
    auto rx = x % p;
    const auto ry = y % p; 
    if (ry == 0)
    {
        assert(rx == 0);
        return 0;
    }
    while ((rx % ry) != 0)
        rx += p;
    return rx / ry; 
}

/**
 * @brief Умножение двух чисел по заданному модулю.
 * @return (x*y) mod m.
 */
inline U128 mult_mod(const U128& x, const U128& y, const U128& m)
{
    assert(m != 0);
    using namespace bignum;
    using U256 = UBig<U128>;
    const U256 z = U256::mult_ext(x, y);
    return (z / m).second;
}

}