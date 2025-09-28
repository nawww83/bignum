#pragma once

#include "i128.hpp"
#include "u128_utils.hpp"
#include <cassert>

namespace i128_utils
{

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
    return u128_utils::isqrt(x.unsigned_part(), exact);
}

/**
 * @brief Целочисленный квадратный корень числа, sqrt(x).
 */
inline I128 isqrt(const I128& x)
{
    [[maybe_unused]] bool exact;
    return isqrt(x, exact);
}

}