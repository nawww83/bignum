#pragma once

#include "i128.hpp"

using namespace bignum::i128;

/**
 * @brief Целочисленный квадратный корень из модуля числа, sqrt(abs(x)).
 * @param exact Точно ли прошло извлечение корня.
 */
inline I128 isqrt(I128 x, bool &exact)
{
    exact = false;
    if (x.is_singular())
        return x;
    x = x.abs();
    I128 result{x >= I128{0, 1} ? ULOW(1) << 32 : ULOW(1) << 16}; // Начальное приближение.
    I128 reg_x[2] {x, I128{0}}; // Регистр сдвига.
    constexpr auto TWO = ULOW{2};
    for (;;) // Метод Ньютона.
    {
        reg_x[1] = reg_x[0];
        reg_x[0] = result;
        const auto &[quotient, remainder] = x / result;
        std::tie(result, std::ignore) = (result + quotient) / TWO;
        if (result.is_zero())
        {
            exact = true;
            return result;
        }
        if (result == reg_x[0])
        {
            exact = remainder.is_zero() && quotient == reg_x[0];
            return result;
        }
        if (result == reg_x[1])
            return reg_x[0];
    }
}

/**
 * @brief Целочисленный квадратный корень из модуля числа, sqrt(abs(x)).
 */
inline I128 isqrt(const I128& x)
{
    [[maybe_unused]] bool exact;
    return isqrt(x, exact);
}