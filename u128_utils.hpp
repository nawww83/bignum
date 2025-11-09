#pragma once

#include "u128.hpp"
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
    return (z / m).second;
}

}