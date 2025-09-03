/**
 * @author nawww83@gmail.com
 * @brief Класс для арифметики 128-битных знаковых целых чисел с переполнением.
 */

#include "u128.hpp"     // U128
#include "sign.hpp"     // Sign
#include "singular.hpp" // Singular

#pragma once

namespace bignum::i128
{
    using U128 = bignum::u128::U128;
    using Sign = sign::Sign;
    using Singular = singular::Singular;

    /**
     * @brief Тип половинки числа.
     */
    using ULOW = uint64_t;

    /**
     * Класс для арифметики 128-битных знаковых целых чисел с переполнением, основанный на классе беззнаковых чисел U128 и знаковом манипуляторе.
     */
    class I128
    {
    public:
        /**
         * @brief Конструктор по умолчанию.
         */
        explicit constexpr I128() = default;

        /**
         * @brief Конструктор с параметром.
         */
        explicit constexpr I128(ULOW low) : mUnsigned{low}
        {
            ;
        }

        /**
         * @brief Конструктор с параметрами.
         */
        explicit constexpr I128(U128 x, Sign sign = false) : mUnsigned{x}, mSign{sign}
        {
            ;
        }

        /**
         * @brief Конструктор с параметрами.
         */
        explicit constexpr I128(U128 x, Sign sign, Singular singularity) : mUnsigned{x}, mSign{sign}, mSingular{singularity}
        {
            ;
        }

        constexpr I128(const I128 &other) = default;

        constexpr I128(I128 &&other) = default;

        constexpr I128 &operator=(const I128 &other) = default;

        /**
         * @brief Оператор сравнения.
         */
        bool operator==(const I128 &other) const
        {
            const auto has_a_singularity = mSingular != other.mSingular;
            return has_a_singularity ? false : *this <=> other == 0;
        }

        /**
         * @brief Остальные операторы, кроме оператора сравнения.
         */
        std::partial_ordering operator<=>(const I128 &other) const
        {
            const auto has_a_singularity = mSingular != other.mSingular;
            if (has_a_singularity)
            {
                return std::partial_ordering::unordered;
            }
            const bool signs_are_equal = mSign() == other.mSign();
            if (signs_are_equal)
            {
                return mSign() ? other.mUnsigned <=> mUnsigned : mUnsigned <=> other.mUnsigned;
            }
            else
            {
                if (mSign())
                {
                    return mUnsigned != U128{0} ? -1 <=> 1 : 1 <=> 1;
                }
                else
                {
                    return mUnsigned != U128{0} ? 1 <=> -1 : 1 <=> 1;
                }
            }
        }

    private:
        /**
         * @brief Беззнаковая часть числа.
         */
        U128 mUnsigned{0};

        /**
         * @brief Знак числа.
         */
        Sign mSign{false};

        /**
         * @brief Признак сингулярности числа: переполнение или "нечисло".
         */
        Singular mSingular{false};
    };
}