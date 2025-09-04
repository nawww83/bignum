/**
 * @author nawww83@gmail.com
 * @brief Класс для арифметики 128-битных беззнаковых целых чисел.
 */

#pragma once

#include <cstdint>   // uint64_t
#include <cassert>   // assert 
#include <string>    // std::string
#include <utility>   // std::exchange
#include <algorithm> // std::min, std::max
#include <tuple>     // std::pair, std::tie

/**
 * @brief Использовать счетчики для проверки количества итераций операции деления.
 */
// #undef USE_DIV_COUNTERS
#define USE_DIV_COUNTERS

namespace bignum::u128
{
#ifdef USE_DIV_COUNTERS
    inline double g_min_loops_when_div = 1. / 0.;
    inline double g_max_loops_when_div = 0;
    inline double g_average_loops_when_div = 0;

    inline double g_all_divs = 0;

    inline double g_min_loops_when_half_div = 1. / 0.;
    inline double g_max_loops_when_half_div = 0;
    inline double g_average_loops_when_half_div = 0;

    inline double g_all_half_divs = 0;
#endif

    /**
     * @brief Тип половинки числа.
     */
    using ULOW = uint64_t;

    /**
     * @brief Число, битовое представление которого состоит из всех единиц.
     */
    inline constexpr ULOW AllUnits()
    {
        return ~ULOW{0};
    }

    /**
     * Массив для формирования цифр по индексам.
     */
    constexpr char DIGITS[10]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    /**
     * Класс для арифметики 128-битных беззнаковых целых чисел, основанный на половинчатом представлении числа.
     */
    class U128
    {
    public:
        /**
         * @brief Конструктор по умолчанию.
         */
        explicit constexpr U128() = default;

        /**
         * @brief Конструктор с параметром.
         */
        explicit constexpr U128(ULOW low) : mLow{low}
        {
            ;
        }

        /**
         * @brief Конструктор с параметрами.
         */
        explicit constexpr U128(ULOW low, ULOW high) : mLow{low}, mHigh{high}
        {
            ;
        }

        constexpr U128(const U128 &other) = default;

        constexpr U128(U128 &&other) = default;

        constexpr U128 &operator=(const U128 &other) = default;

        /**
         * @brief Оператор равно.
         */
        bool operator==(const U128 &other) const
        {
            return mLow == other.mLow && mHigh == other.mHigh;
        }

        /**
         * @brief Оператор сравнения.
         */
        std::partial_ordering operator<=>(const U128 &other) const
        {
            auto high_cmp = mHigh <=> other.mHigh;
            return high_cmp != 0 ? high_cmp : mLow <=> other.mLow;
        }

        /**
         * @brief Оператор сдвига влево. При больших сдвигах дает ноль.
         */
        U128 operator<<(uint32_t shift) const
        {
            if (shift >= 128u)
            {
                return U128{0};
            }
            U128 result = *this;
            int ishift = shift;
            ULOW L = 0;
            if (ishift < 64)
            {
                L = ishift == 0 ? L : result.mLow >> (64 - ishift);
            }
            else
            {
                result.mHigh = std::exchange(result.mLow, 0);
                ishift -= 64;
            }
            result.mLow <<= ishift;
            result.mHigh <<= ishift;
            result.mHigh |= L;
            return result;
        }

        /**
         * @brief Оператор сдвига влево.
         */
        U128 &operator<<=(uint32_t shift)
        {
            *this = *this << shift;
            return *this;
        }

        /**
         * @brief Оператор сдвига вправо. При больших сдвигах дает ноль.
         */
        U128 operator>>(uint32_t shift) const
        {
            if (shift >= 128u)
            {
                return U128{0};
            }
            U128 result = *this;
            int ishift = shift;
            if (ishift < 64)
            {
                ULOW mask = AllUnits();
                mask <<= ishift;
                mask = ~mask;
                const ULOW H = result.mHigh & mask;
                result.mLow >>= ishift;
                result.mHigh >>= ishift;
                result.mLow |= ishift == 0 ? H : H << (64 - ishift);
            }
            else
            {
                result.mLow = std::exchange(result.mHigh, 0);
                result.mLow >>= (ishift - 64);
            }
            return result;
        }

        /**
         * @brief Оператор сдвига вправо.
         */
        U128 &operator>>=(uint32_t shift)
        {
            *this = *this >> shift;
            return *this;
        }

        /**
         * @brief Оператор побитового И.
         */
        U128 operator&(const U128 &mask) const
        {
            U128 result = *this;
            result.mLow &= mask.mLow;
            result.mHigh &= mask.mHigh;
            return result;
        }

        /**
         * @brief Оператор побитового И.
         */
        U128 &operator&=(const U128 &mask)
        {
            *this = *this & mask;
            return *this;
        }

        /**
         * @brief Оператор побитового ИЛИ.
         */
        U128 operator|(const U128 &mask) const
        {
            U128 result = *this;
            result.mLow |= mask.mLow;
            result.mHigh |= mask.mHigh;
            return result;
        }

        /**
         * @brief Оператор побитового ИЛИ.
         */
        U128 &operator|=(const U128 &mask)
        {
            *this = *this | mask;
            return *this;
        }

        /**
         * @brief Оператор исключающего ИЛИ.
         */
        U128 operator^(const U128 &mask) const
        {
            U128 result = *this;
            result.mLow ^= mask.mLow;
            result.mHigh ^= mask.mHigh;
            return result;
        }

        /**
         * @brief Оператор исключающего ИЛИ.
         */
        U128 &operator^=(const U128 &mask)
        {
            *this = *this ^ mask;
            return *this;
        }

        /**
         * @brief Оператор инверсии битов. Не влияет на знак.
         */
        U128 operator~() const
        {
            U128 result = *this;
            result.mLow = ~result.mLow;
            result.mHigh = ~result.mHigh;
            return result;
        }

        /**
         * @brief Оператор суммирования.
         */
        U128 operator+(const U128 &Y) const
        {
            const U128 &X = *this;
            U128 result{X.mLow + Y.mLow, X.mHigh + Y.mHigh};
            const ULOW carry = result.mLow < std::min(X.mLow, Y.mLow);
            result.mHigh += carry;
            return result;
        }

        /**
         *
         */
        U128 &operator+=(const U128 &Y)
        {
            *this = *this + Y;
            return *this;
        }

        /**
         * @brief Оператор вычитания.
         */
        U128 operator-(const U128 &Y) const
        {
            const U128 &X = *this;
            if (X >= Y)
            {
                const ULOW high{X.mHigh - Y.mHigh - (X.mLow < Y.mLow ? ULOW{1} : ULOW{0})};
                return U128{X.mLow - Y.mLow, high};
            }
            return (X + (U128::get_max_value() - Y)).inc();
        }

        /**
         *
         */
        U128 &operator-=(const U128 &Y)
        {
            *this = *this - Y;
            return *this;
        }

        /**
         * @brief Инкремент числа.
         * @return Число + 1.
         */
        U128 &inc()
        {
            *this = *this + U128{1};
            return *this;
        }

        /**
         * @brief Декремент числа.
         * @return Число - 1.
         */
        U128 &dec()
        {
            *this = *this - U128{1};
            return *this;
        }

        /**
         * @brief Оператор умножения.
         */
        U128 operator*(const U128 &Y) const
        {
            const U128 &X = *this;
            // x*y = (a + w*b)(c + w*d) = ac + w*(ad + bc) + w*w*bd = (ac + w*(ad + bc)) mod 2^128;
            const U128 ac = mult64(X.mLow, Y.mLow);
            const U128 ad = mult64(X.mLow, Y.mHigh);
            const U128 bc = mult64(X.mHigh, Y.mLow);
            U128 result = ad + bc;
            result <<= 64;
            result += ac;
            return result;
        }

        /**
         *
         */
        U128 &operator*=(const U128 &Y)
        {
            *this = *this * Y;
            return *this;
        }

        /**
         * @brief Половинчатый оператор умножения.
         */
        U128 operator*(const ULOW &Y) const
        {
            const U128 &X = *this;
            // x*y = (a + w*b)(c + w*0) = ac + w*(0 + bc) = (ac + w*bc) mod 2^128;
            U128 result{mult64(X.mHigh, Y)};
            result <<= 64;
            result += mult64(X.mLow, Y);
            return result;
        }

        /**
         *
         */
        U128 &operator*=(const ULOW &Y)
        {
            *this = *this * Y;
            return *this;
        }

        /**
         * @brief Половинчатый оператор полного деления.
         * @details Авторский метод итеративного деления "широкого" числа на "узкое".
         * @return Частное от деления и остаток.
         */
        std::pair<U128, U128> operator/(const ULOW &Y) const
        {
            assert(Y != 0);
            const U128 &X = *this;
            constexpr ULOW MAX_ULOW = AllUnits();
            ULOW Q = X.mHigh / Y;
            ULOW R = X.mHigh % Y;
            ULOW N = R * (MAX_ULOW / Y) + (X.mLow / Y);
            U128 result{N, Q};
            U128 E = X - result * Y; // Остаток от деления.
#ifdef USE_DIV_COUNTERS
            g_all_half_divs++;
            double loops = 0;
#endif
            for (;;)
            {
#ifdef USE_DIV_COUNTERS
                loops++;
#endif
                Q = E.mHigh / Y;
                R = E.mHigh % Y;
                N = R * (MAX_ULOW / Y) + (E.mLow / Y);
                const U128 tmp{N, Q};
                if (tmp == U128{0})
                    break;
                result += tmp;
                E -= tmp * Y;
            }
#ifdef USE_DIV_COUNTERS
            g_average_loops_when_half_div += (loops - g_average_loops_when_half_div) / g_all_half_divs;
            g_max_loops_when_half_div = std::max(g_max_loops_when_half_div, loops);
            g_min_loops_when_half_div = std::min(g_min_loops_when_half_div, loops);
#endif
            return std::make_pair(result, E);
        }

        /**
         *
         */
        std::pair<U128, U128> operator/=(const ULOW &Y)
        {
            U128 remainder;
            std::tie(*this, remainder) = *this / Y;
            return std::make_pair(*this, remainder);
        }

        /**
         * @brief Оператор полного деления.
         * @details Авторский метод деления двух "широких" чисел, состоящих из двух половинок - "узких" чисел.
         * Отсутствует "раскачка" алгоритма для "плохих" случаев деления: (A*w + B)/(1*w + D).
         * @return Частное от деления и остаток.
         */
        std::pair<U128, U128> operator/(const U128 &other) const
        {
            assert(other != U128{0});
            const U128 &X = *this;
            const U128 &Y = other;
            if (Y.mHigh == 0)
            {
                auto result = X / Y.mLow;
                return result;
            }
            constexpr ULOW MAX_ULOW = AllUnits();
            const ULOW Q = X.mHigh / Y.mHigh;
            const ULOW R = X.mHigh % Y.mHigh;
            const ULOW Delta = MAX_ULOW - Y.mLow;
            const U128 &DeltaQ = mult64(Delta, Q);
            U128 W1{U128{0, R} - U128{0, Q}};
            W1 += DeltaQ;
#ifdef USE_DIV_COUNTERS
            g_all_divs++;
            double loops = 0;
#endif
            // ******* Unsigned hack. Это только для беззнаковых чисел.
            // Для учета случаев, когда W1 отрицательное, если бы был знак.
            for (; W1 >= Y;)
            {
#ifdef USE_DIV_COUNTERS
                loops++;
#endif
                W1 += Y;
            }
            // ******* Unsigned hack.
            const ULOW C1 = (Y.mHigh < MAX_ULOW) ? Y.mHigh + ULOW{1} : MAX_ULOW;
            const ULOW W2 = MAX_ULOW - Delta / C1;
            auto [Quotient, _] = W1 / W2;
            std::tie(Quotient, std::ignore) = Quotient / C1;
            U128 result = U128{Q} + Quotient;
            const U128 &N = Y * result.mLow;
            U128 Error{X - N};
            while (Error >= Y)
            {
#ifdef USE_DIV_COUNTERS
                loops++;
#endif
                result.dec();
                Error += Y;
            }
#ifdef USE_DIV_COUNTERS
            g_average_loops_when_div += (loops - g_average_loops_when_div) / g_all_divs;
            g_max_loops_when_div = std::max(g_max_loops_when_div, loops);
            g_min_loops_when_div = std::min(g_min_loops_when_div, loops);
#endif
            return std::make_pair(result, Error);
        }

        /**
         *
         */
        std::pair<U128, U128> operator/=(const U128 &Y)
        {
            U128 remainder;
            std::tie(*this, remainder) = *this / Y;
            return std::make_pair(*this, remainder);
        }

        /**
         * @brief Получить максимальное значение 128-битного числа.
         */
        static constexpr U128 get_max_value()
        {
            return U128{AllUnits(), AllUnits()};
        }

        /**
         * @brief Умножение двух 64-битных чисел с расширением до 128-битного числа.
         * @details Авторский алгоритм умножения.
         */
        static U128 mult64(ULOW x, ULOW y)
        {
            constexpr int QUORTER_WIDTH = 32; // Четверть ширины 128-битного числа.
            constexpr ULOW MASK = (ULOW{1} << QUORTER_WIDTH) - 1u;
            const ULOW x_low = x & MASK;
            const ULOW y_low = y & MASK;
            const ULOW x_high = x >> QUORTER_WIDTH;
            const ULOW y_high = y >> QUORTER_WIDTH;
            const ULOW t1 = x_low * y_low;
            const ULOW t = t1 >> QUORTER_WIDTH;
            const ULOW t21 = x_low * y_high;
            const ULOW q = t21 >> QUORTER_WIDTH;
            const ULOW p = t21 & MASK;
            const ULOW t22 = x_high * y_low;
            const ULOW s = t22 >> QUORTER_WIDTH;
            const ULOW r = t22 & MASK;
            const ULOW t3 = x_high * y_high;
            U128 result{t1};
            const ULOW div = (q + s) + ((p + r + t) >> QUORTER_WIDTH);
            const auto &p1 = t21 << QUORTER_WIDTH;
            const auto &p2 = t22 << QUORTER_WIDTH;
            const ULOW mod{p1 + p2};
            result.mLow += mod;
            result.mHigh += div;
            result.mHigh += t3;
            return result;
        }

        /**
         * @brief Специальный метод деления на 10 для формирования строкового представления числа.
         */
        U128 div10() const
        {
            const U128 &X = *this;
            constexpr auto TEN = ULOW{10};
            ULOW Q = X.mHigh / TEN;
            ULOW R = X.mHigh % TEN;
            constexpr ULOW MAX_ULOW = AllUnits();
            ULOW N = R * (MAX_ULOW / TEN) + (X.mLow / TEN);
            U128 result{N, Q};
            const U128 &tmp = result * TEN;
            U128 E{X - tmp};
            while (E.mHigh != 0 || E.mLow >= TEN)
            {
                Q = E.mHigh / TEN;
                R = E.mHigh % TEN;
                N = R * (MAX_ULOW / TEN) + (E.mLow / TEN);
                U128 tmp{N, Q};
                result += tmp;
                E -= tmp * TEN;
            }
            return result;
        }

        /**
         * @brief Специальный метод нахождения остатка от деления на 10 для формирования строкового представления числа.
         */
        int mod10() const
        {
            constexpr ULOW MAX_ULOW = AllUnits();
            constexpr int multiplier_mod10 = MAX_ULOW % 10 + 1;
            return ((mLow % 10) + multiplier_mod10 * (mHigh % 10)) % 10;
        }

        /**
         * @brief Возвращает строковое представление числа.
         */
        std::string value() const
        {
            std::string result;
            U128 X = *this;
            while (X != U128{0})
            {
                const int d = X.mod10();
                if (d < 0)
                    return result;
                result.push_back(DIGITS[d]);
                X = X.div10();
            }
            std::reverse(result.begin(), result.end());
            return result.length() != 0 ? result : "0";
        }

    private:
        /**
         * @brief Младшая половина числа.
         */
        ULOW mLow{0};

        /**
         * @brief Старшая половина числа.
         */
        ULOW mHigh{0};
    };

}