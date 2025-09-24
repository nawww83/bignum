#pragma once

#include "i128.hpp"

using namespace bignum::i128;

/**
 * @brief Целочисленный квадратный корень числа, sqrt(x).
 * @param exact Точно ли прошло извлечение корня.
 */
inline I128 isqrt(const I128& x, bool &exact)
{
    exact = false;
    if (x.is_singular())
        return x;
    assert(x.is_nonegative());
    if (x.is_zero())
    {
        exact = true;
        return x;
    }
    const U128& X = x.unsigned_part();
    U128 result{ULOW{1ull} << (x.bit_length() / 2)}; // Начальное приближение.
    U128 reg_x[] {X, U128{0}}; // Регистр сдвига.
    constexpr auto TWO = ULOW{2};
    for (;;) // Метод Ньютона.
    {
        reg_x[1] = reg_x[0] = result;
        const auto &[quotient, remainder] = X / result;
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
 * @brief Целочисленный квадратный корень из модуля числа, sqrt(abs(x)).
 */
inline I128 isqrt(const I128& x)
{
    [[maybe_unused]] bool exact;
    return isqrt(x, exact);
}