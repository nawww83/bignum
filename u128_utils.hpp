#pragma once

#include "u128.hpp"
#include "i128.hpp"
#include <cassert>
#include <utility> // std::pair
#include "ubig.hpp"

namespace u128_utils
{

using namespace bignum::u128;

/**
 * @brief Целочисленный квадратный корень числа, sqrt(x).
 * @param exact Точно ли прошло извлечение корня.
 */
inline U128 isqrt(const U128& x, bool &exact)
{
    exact = false;
    if (x == 0)
    {
        exact = true;
        return x;
    }
    auto result = U128{1} << (x.bit_length() / 2); // Начальное приближение.
    U128 reg_x[] {x, U128{0}}; // Регистр сдвига.
    constexpr auto TWO = ULOW{2};
    for (;;) // Метод Ньютона.
    {
        reg_x[1] = reg_x[0];
        reg_x[0] = result;
        const auto &[quotient, remainder] = x / result;
        std::tie(result, std::ignore) = (result + quotient) / TWO;
        if (result == reg_x[0])
        {
            exact = remainder == U128{0} && quotient == result;
            return result;
        }
        if (result == reg_x[1])
            return reg_x[0];
    }
}

/**
 * @brief Целочисленный квадратный корень числа, sqrt(x).
 */
inline U128 isqrt(const U128& x)
{
    [[maybe_unused]] bool exact;
    return isqrt(x, exact);
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
 * @brief Быстрый алгоритм вычисления целочисленной степени числа.
 * @details Предполагается, что переполнения не будет, т.е. расчет идет по модулю 2^128.
 * @param x
 * @param y
 * @return
 */
inline U128 int_power_fast(const U128& x, unsigned y)
{
    U128 exponent {y};
    U128 base = x;
    U128 result = 1;
    while (exponent != 0)
    {
        if ((exponent & 1) == 1)
            result *= base;
        exponent >>= 1;
        base *= base;
    }
    return result;
}


/**
 * @brief Целочисленный корень m-й степени из числа.
 * @param m Степень корня.
 */
inline U128 nroot(const U128& x, unsigned m)
{
    assert(m > 0);
    if (x < 2)
        return x;
    if (m == 1)
        return x;
    int d = x.bit_length() / m; // m > 1
    const int r = x.bit_length() % m;
    d = r == 0 ? d : d + 1; // ceil(bit_length(x) / m)
    auto result = U128{1} << d; // Начальное приближение (сверху).
    U128 m_ext {m};
    U128 old_result = result;
    for (;;) // Метод Ньютона.
    {
        const auto power = int_power_fast(result, m - 1);
        result = (((m_ext - 1) * result + (x / power).first) / m_ext).first;
        if (result >= old_result) { // Условие простое благодаря выбору начального приближения сверху.
            result = old_result;
            break;
        }
        old_result = result;
    }
    return result;
}

/**
 * @brief Является ли число x квадратичным вычетом по модулю p.
 */
inline bool is_quadratic_residue(const U128& x, const U128& p)
{
    assert(p != 0);
    const auto& [_, rx] = x / p;
    U128 y2 = 0;
    for (U128 y = 0; y < p; y.inc())
    {                
        if (const auto& [_, ry2] = y2 / p; ry2 == rx)
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
        const auto [q, _] = a / m;
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
    const auto& rx = (x / p).second;
    U128 y2 = 0;
    for (U128 y = 0; y < p; y.inc())
    {
        if (const auto& ry2 = (y2 / p).second; ry2 == rx)
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
    auto rx = (x / p).second;
    const auto& ry = (y / p).second; 
    if (ry == 0)
    {
        assert(rx == 0);
        return 0;
    }
    while ((rx / ry).second != 0)
        rx += p;
    return (rx / ry).first; 
}

/**
 * @brief Умножение двух чисел по заданному модулю.
 * @return (x*y) mod m.
 */
inline U128 mult_mod(const U128& x, const U128& y, const U128& m)
{
    assert(m != 0);
    using namespace bignum::ubig;
    using U256 = UBig<U128, 256>;
    const U256& z = U256::mult_ext(x, y);
    return z % m;
}

}