/**
 * @author nawww83@gmail.com
 * @brief Класс для арифметики 128-битных знаковых целых чисел с переполнением.
 */

#include <cstdint>      // uint16_t, uint8_t
#include "u128.hpp"     // U128
#include "sign.hpp"     // Sign
#include "singular.hpp" // Singular

#pragma once

namespace bignum::i128
{
    using U128 = bignum::u128::U128;
    using Sign = sign::Sign<uint16_t>;
    using Singular = singular::Singular<uint8_t>;

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

        bool is_singular() const
        {
            return mSingular();
        }

        bool is_overflow() const
        {
            return mSingular.is_overflow() && !mSingular.is_nan();
        }

        bool is_nan() const
        {
            return mSingular.is_nan() && !mSingular.is_overflow();
        }

        bool is_zero() const
        {
            return mUnsigned == U128{0} && !is_singular();
        }

        bool is_unit() const
        {
            return mUnsigned == U128{1} && !mSign() && !is_singular();
        }

        /**
         * @brief x < 0
         */
        bool is_negative() const
        {
            return !is_zero() && mSign() && !is_singular();
        }

        /**
         * @brief x > 0
         */
        bool is_positive() const
        {
            return !is_zero() && !mSign() && !is_singular();
        }

        /**
         * @brief x >= 0
         */
        bool is_nonegative() const
        {
            return is_positive() || is_zero();
        }

        void set_overflow()
        {
            mSingular.set_overflow();
        }

        void set_nan()
        {
            mSingular.set_nan();
        }

        /**
         * @brief Оператор сдвига влево. При больших сдвигах дает ноль. Не меняет знак.
         */
        I128 operator<<(uint32_t shift) const
        {
            I128 result = *this;
            result.mUnsigned <<= shift;
            return result;
        }

        /**
         * @brief Оператор сдвига влево.
         */
        I128 &operator<<=(uint32_t shift)
        {
            *this = *this << shift;
            return *this;
        }

        /**
         * @brief Оператор сдвига вправо. При больших сдвигах дает ноль. Не меняет знак.
         */
        I128 operator>>(uint32_t shift) const
        {
            I128 result = *this;
            result.mUnsigned <<= shift;
            return result;
        }

        /**
         * @brief Оператор сдвига вправо.
         */
        I128 &operator>>=(uint32_t shift)
        {
            *this = *this >> shift;
            return *this;
        }

        /**
         * @brief Оператор побитового И. Сохраняет знак.
         */
        I128 operator&(const I128 &mask) const
        {
            I128 result = *this;
            result.mUnsigned &= mask.mUnsigned;
            return result;
        }

        /**
         * @brief Оператор побитового И.
         */
        I128 &operator&=(const I128 &mask)
        {
            *this = *this & mask;
            return *this;
        }

        /**
         * @brief Оператор побитового ИЛИ. Сохраняет знак.
         */
        I128 operator|(const I128 &mask) const
        {
            I128 result = *this;
            result.mUnsigned |= mask.mUnsigned;
            return result;
        }

        /**
         * @brief Оператор побитового ИЛИ.
         */
        I128 &operator|=(const I128 &mask)
        {
            *this = *this | mask;
            return *this;
        }

        /**
         * @brief Оператор исключающего ИЛИ. Сохраняет знак.
         */
        I128 operator^(const I128 &mask) const
        {
            I128 result = *this;
            result.mUnsigned ^= mask.mUnsigned;
            return result;
        }

        /**
         * @brief Оператор исключающего ИЛИ.
         */
        I128 &operator^=(const I128 &mask)
        {
            *this = *this ^ mask;
            return *this;
        }

        /**
         * @brief Оператор инверсии битов. Сохраняет знак.
         */
        I128 operator~() const
        {
            I128 result = *this;
            result.mUnsigned = ~result.mUnsigned;
            return result;
        }

        /**
         * @brief Возвращает абсолютное значение числа.
         */
        I128 abs() const
        {
            I128 result = *this;
            result.mSign.set_sign(false);
            return result;
        }

        /**
         * @brief Оператор суммирования.
         */
        I128 operator+(const I128 &rhs) const
        {
            I128 X = *this;
            I128 Y = rhs;
            if (X.is_singular())
            {
                return X;
            }
            if (Y.is_singular())
            {
                X.mSingular = Y.mSingular;
                return X;
            }
            I128 result;
            if (X.is_negative() && !Y.is_negative())
            {
                X.mSign.set_sign(false);
                result = Y - X;
                return result;
            }
            if (!X.is_negative() && Y.is_negative())
            {
                Y.mSign.set_sign(false);
                result = X - Y;
                return result;
            }
            result.mUnsigned = X.mUnsigned + Y.mUnsigned;
            if (X.is_negative() && Y.is_negative()) {
                result.mSign.set_sign(true);
            }
            if (result.mUnsigned < std::min(X.mUnsigned, Y.mUnsigned)) {
                result.mSingular.set_overflow();
            }
            return result;
        }

        /**
         * @brief Оператор суммирования.
         */
        I128 &operator+=(const I128 &Y)
        {
            *this = *this + Y;
            return *this;
        }

        I128 operator-(const I128& rhs) const
        {
            I128 X = *this;
            I128 Y = rhs;
            if (X.is_singular())
            {
                return X;
            }
            if (Y.is_singular())
            {
                X.mSingular = Y.mSingular;
                return X;
            }
            I128 result;
            if (X.is_negative() && !Y.is_negative())
            {
                Y.mSign.set_sign(true);
                result = Y + X;
                return result;
            }
            if (!X.is_negative() && Y.is_negative())
            {
                Y.mSign.set_sign(false);
                result = X + Y;
                return result;
            }
            if (X.is_negative() && Y.is_negative())
            {
                Y.mSign.set_sign(false);
                X.mSign.set_sign(false);
                result = Y - X;
                return result;
            }
            if (X.is_zero())
            {
                result = Y;
                -result.mSign;
                return result;
            }
            if (X.mUnsigned >= Y.mUnsigned) {
                result.mUnsigned = X.mUnsigned - Y.mUnsigned;
            } else {
                result.mUnsigned = Y.mUnsigned - X.mUnsigned;
                result.mSign.set_sign(true);
            }
            return result;
        }

        I128 &operator-=(const I128& other)
        {
            *this = *this - other;
            return *this;
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