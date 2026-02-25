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
#include "defines.h"

#include <bit>
#include <compare>

#if defined(_MSC_VER) && (_MSC_VER >= 1920) && defined(_M_X64)
#define USE_MSVC_INTRINSICS_DIVISION
#endif

#if defined(_MSC_VER) && defined(_M_X64)
#define USE_MSVC_INTRINSICS
#endif

#ifdef USE_MSVC_INTRINSICS_DIVISION
#include <immintrin.h>
#endif

#ifdef USE_MSVC_INTRINSICS
#include <intrin.h>
#pragma intrinsic(_umul128)
#endif

namespace bignum::u128
{
    using u64 = uint64_t;
    class U128
    {
        u64 mLow{0};
        u64 mHigh{0};

    public:
        // --- Конструкторы ---
        constexpr U128() noexcept = default;
        constexpr U128(u64 low, u64 high) noexcept : mLow{low}, mHigh{high} {}
        constexpr U128(u64 low) noexcept : mLow{low}, mHigh{0} {}

        // --- Доступ к данным ---
        [[nodiscard]] constexpr u64 low() const noexcept { return mLow; }
        [[nodiscard]] constexpr u64 high() const noexcept { return mHigh; }
        [[nodiscard]] constexpr u64 &low() noexcept { return mLow; }
        [[nodiscard]] constexpr u64 &high() noexcept { return mHigh; }

        // --- Сравнение ---
        bool operator==(const U128 &other) const noexcept
        {
            return mLow == other.mLow && mHigh == other.mHigh;
        }
        std::strong_ordering operator<=>(const U128 &other) const noexcept
        {
            if (auto cmp = mHigh <=> other.mHigh; cmp != 0)
                return cmp;
            return mLow <=> other.mLow;
        }

        // --- Сложение ---
        constexpr U128 &operator+=(const U128 &other) noexcept
        {
#if defined(__SIZEOF_INT128__)
            unsigned __int128 val = to_u128() + other.to_u128();
            mLow = static_cast<u64>(val);
            mHigh = static_cast<u64>(val >> 64);
#else
            u64 old_low = mLow;
            mLow += other.mLow;

            // Сначала определяем наличие переноса из младшей части
            u64 carry = (mLow < old_low) ? 1ull : 0ull;

            // Затем прибавляем всё к старшей части
            mHigh += other.mHigh + carry;
#endif
            return *this;
        }
        U128 operator+(const U128 &other) const noexcept { return U128(*this) += other; }

        // --- Вычитание ---
        constexpr U128 &operator-=(const U128 &other) noexcept
        {
#if defined(__SIZEOF_INT128__)
            auto val = to_u128() - other.to_u128();
            mLow = static_cast<u64>(val);
            mHigh = static_cast<u64>(val >> 64);
#else
            u64 old_low = mLow;
            mLow -= other.mLow;
            mHigh -= other.mHigh + (old_low < other.mLow ? 1ull : 0ull);
#endif
            return *this;
        }
        U128 operator-(const U128 &other) const noexcept { return U128(*this) -= other; }

        /**
         * @brief Унарный минус (отрицание).
         * @details В беззнаковой арифметике возвращает (2^128 - *this).
         */
        [[nodiscard]] constexpr U128 operator-() const noexcept
        {
#if defined(__SIZEOF_INT128__)
            unsigned __int128 val = to_u128();
            val = -val; // Нативная поддержка дополнения до двух
            return {static_cast<u64>(val), static_cast<u64>(val >> 64)};
#else
            // Побитовая инверсия + 1 (стандартный алгоритм для дополнения до двух)
            U128 res = ~(*this);
            return ++res;
#endif
        }

        // --- Умножение ---
        U128 &operator*=(const U128 &other) noexcept
        {
            *this = (*this * other);
            return *this;
        }
        U128 operator*(const U128 &other) const noexcept
        {
#if defined(__SIZEOF_INT128__)
            auto res = to_u128() * other.to_u128();
            return {static_cast<u64>(res), static_cast<u64>(res >> 64)};
#else
            U128 res = mult_ext(mLow, other.mLow);
            res.mHigh += (mLow * other.mHigh) + (mHigh * other.mLow);
            return res;
#endif
        }

        static U128 mult_ext(u64 x, u64 y) noexcept
        {
#if defined(__SIZEOF_INT128__)
            unsigned __int128 res = static_cast<unsigned __int128>(x) * y;
            return {static_cast<u64>(res), static_cast<u64>(res >> 64)};
#elif defined(_MSC_VER)
            u64 hi, lo = _umul128(x, y, &hi);
            return {lo, hi};
#else
           return mult_ext_manual(x, y);
#endif
        }

        /**
         * @brief Возведение в квадрат 64-битного числа с расширением до 128-битного числа.
         */
        static U128 square_ext(u64 x) noexcept
        {
#if defined(__SIZEOF_INT128__)
            unsigned __int128 res = static_cast<unsigned __int128>(x) * static_cast<unsigned __int128>(x);
            return {static_cast<u64>(res), static_cast<u64>(res >> 64)};
#elif defined(USE_MSVC_INTRINSICS)
            u64 hi, lo = _umul128(x, x, &hi);
            return {lo, hi};
#else
           return square_ext_manual(x);
#endif
        }

        // --- Деление и остаток ---
        U128 &operator/=(const U128 &other) noexcept
        {
            return *this = divide<true, false>(*this, other, nullptr);
        }

        U128 operator/(const U128 &other) const noexcept
        {
            return divide<true, false>(*this, other, nullptr);
        }

        U128 &operator%=(const U128 &other) noexcept
        {
            U128 rem;
            divide<false, true>(*this, other, &rem);
            return *this = rem;
        }

        U128 operator%(const U128 &other) const noexcept
        {
            U128 rem;
            divide<false, true>(*this, other, &rem);
            return rem;
        }

        // --- Сдвиги ---
        U128 &operator<<=(uint32_t s) noexcept
        {
            if (s >= 128)
            {
                mLow = mHigh = 0;
            }
            else if (s >= 64)
            {
                mHigh = mLow << (s - 64);
                mLow = 0;
            }
            else if (s > 0)
            {
                mHigh = (mHigh << s) | (mLow >> (64 - s));
                mLow <<= s;
            }
            return *this;
        }
        U128 operator<<(uint32_t s) const noexcept { return U128(*this) <<= s; }

        U128 &operator>>=(uint32_t s) noexcept
        {
            if (s >= 128)
            {
                mLow = mHigh = 0;
            }
            else if (s >= 64)
            {
                mLow = mHigh >> (s - 64);
                mHigh = 0;
            }
            else if (s > 0)
            {
                mLow = (mLow >> s) | (mHigh << (64 - s));
                mHigh >>= s;
            }
            return *this;
        }
        U128 operator>>(uint32_t s) const noexcept { return U128(*this) >>= s; }

        // --- Инкремент / Декремент ---
        U128 &operator++() noexcept
        {
            if (++mLow == 0)
                ++mHigh;
            return *this;
        }
        U128 operator++(int) noexcept
        {
            U128 tmp(*this);
            ++(*this);
            return tmp;
        }
        U128 &operator--() noexcept
        {
            if (mLow-- == 0)
                --mHigh;
            return *this;
        }
        U128 operator--(int) noexcept
        {
            U128 tmp(*this);
            --(*this);
            return tmp;
        }

        // --- Логические ---
        constexpr U128 operator~() const noexcept { return {~mLow, ~mHigh}; }
        constexpr U128 &operator&=(const U128 &other) noexcept
        {
            mLow &= other.mLow;
            mHigh &= other.mHigh;
            return *this;
        }
        constexpr U128 &operator|=(const U128 &other) noexcept
        {
            mLow |= other.mLow;
            mHigh |= other.mHigh;
            return *this;
        }
        constexpr U128 &operator^=(const U128 &other) noexcept
        {
            mLow ^= other.mLow;
            mHigh ^= other.mHigh;
            return *this;
        }

        // --- Побитовые операторы (Binary) ---
        [[nodiscard]] constexpr U128 operator&(const U128 &other) const noexcept { return U128(*this) &= other; }
        [[nodiscard]] constexpr U128 operator|(const U128 &other) const noexcept { return U128(*this) |= other; }
        [[nodiscard]] constexpr U128 operator^(const U128 &other) const noexcept { return U128(*this) ^= other; }

        // --- Логические проверки ---
        // Позволяет писать: if (val), if (!val), val && other, val ||宣 other
        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return mLow != 0 || mHigh != 0;
        }

        [[nodiscard]] constexpr bool operator!() const noexcept
        {
            return mLow == 0 && mHigh == 0;
        }

        /**
         * @brief Возвращает максимально возможное значение U128 (2^128 - 1).
         */
        [[nodiscard]] static constexpr U128 max() noexcept
        {
            return U128{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
        }

        /**
         * @brief Возвращает минимальное значение (0).
         */
        [[nodiscard]] static constexpr U128 min() noexcept
        {
            return U128{0, 0};
        }

        /**
         * @brief Подсчет количества ведущих нулей (Count Leading Zeros).
         * @return Количество нулевых бит слева (от 0 до 128).
         */
        [[nodiscard]] constexpr int countl_zero() const noexcept
        {
            if (mHigh != 0)
            {
                return std::countl_zero(mHigh);
            }
            return 64 + std::countl_zero(mLow);
        }

        /**
         * @brief Подсчет количества завершающих нулей (Count Trailing Zeros).
         * @return Количество нулевых бит справа (от 0 до 128).
         */
        [[nodiscard]] constexpr int countr_zero() const noexcept
        {
            if (mLow != 0)
            {
                return std::countr_zero(mLow);
            }
            return 64 + std::countr_zero(mHigh);
        }

        /**
         * @brief Подсчет количества установленных бит (Population Count).
         * @return Общее количество бит, равных 1.
         */
        [[nodiscard]] constexpr int popcount() const noexcept
        {
            return std::popcount(mHigh) + std::popcount(mLow);
        }

        /**
         * @brief Минимальное количество бит для представления числа.
         */
        [[nodiscard]] constexpr uint32_t bit_width() const noexcept
        {
            return 128 - countl_zero();
        }

        /**
         * @brief Возвращает остаток от деления числа на 10.
         * @return Цифра от 0 до 9.
         */
        [[nodiscard]] constexpr int mod10() const noexcept
        {
#if defined(__SIZEOF_INT128__)
            // Для GCC/Clang это превратится в одну инструкцию деления или оптимизированное умножение
            unsigned __int128 val = (static_cast<unsigned __int128>(mHigh) << 64) | mLow;
            return static_cast<int>(val % 10u);
#else
            // Для MSVC используем наш быстрый путь деления 128/64
            u64 rem;
            // Нам не нужно частное, поэтому вызываем только логику получения остатка
            _udiv128(mHigh % 10u, mLow, 10u, &rem);
            return static_cast<int>(rem);
#endif
        }

        /**
         * @brief Быстрое деление на 10 (целочисленное).
         * Изменяет объект на месте.
         */
        constexpr void div10() noexcept
        {
#if defined(__SIZEOF_INT128__)
            // Ветка для GCC/Clang: компилятор превратит это в "магическое умножение"
            unsigned __int128 val = to_u128();
            val /= 10u;
            mLow = static_cast<u64>(val);
            mHigh = static_cast<u64>(val >> 64);
#else
            // Ветка для MSVC: используем цепочку из двух аппаратных делений 128/64
            u64 dummy_rem;
            // 1. Делим старшую часть: mHigh / 10
            // Остаток от этого деления (mHigh % 10) становится префиксом для младшей части
            u64 new_high = _udiv128(0, mHigh, 10u, &dummy_rem);

            // 2. Делим младшую часть вместе с остатком от старшей: (rem:mLow) / 10
            u64 new_low = _udiv128(dummy_rem, mLow, 10u, &dummy_rem);

            mHigh = new_high;
            mLow = new_low;
#endif
        }

        template <bool make_quotient, bool make_remainder>
        static U128 divide(const U128 &dividend, const U128 &divisor, U128 *rem_out)
        {
#if defined(__SIZEOF_INT128__)
            unsigned __int128 a = dividend.to_u128(), b = divisor.to_u128();
            if constexpr (make_remainder)
                if (rem_out)
                {
                    unsigned __int128 r = a % b;
                    *rem_out = {static_cast<u64>(r), static_cast<u64>(r >> 64)};
                }
            if constexpr (make_quotient)
            {
                unsigned __int128 q = a / b;
                return {static_cast<u64>(q), static_cast<u64>(q >> 64)};
            }
            return {0, 0};
#else
            if (divisor.mHigh == 0)
            {
                u64 r0, r1;
                u64 q1 = _udiv128(0, dividend.mHigh, divisor.mLow, &r0);
                u64 q0 = _udiv128(r0, dividend.mLow, divisor.mLow, &r1);
                if constexpr (make_remainder)
                    if (rem_out)
                        *rem_out = {r1, 0};
                return {q0, q1};
            }
            u32 s = std::countl_zero(divisor.mHigh);
            U128 v = divisor << s, u = dividend;
            u64 r_tmp;
            u64 q_h = _udiv128(u.mHigh >> (64 - s), (u.mHigh << s) | (u.mLow >> (64 - s)), v.mHigh, &r_tmp);
            U128 q_res{q_h, 0}, prod = q_res * divisor;
            while (prod > dividend)
            {
                prod -= divisor;
                q_res.mLow--;
            }
            if constexpr (make_remainder)
                if (rem_out)
                    *rem_out = dividend - prod;
            return q_res;
#endif
        }
        /**
         * @brief Преобразование в десятичную строку через быстрые цепочки деления.
         */
        [[nodiscard]] std::string toString() const
        {
            if (mHigh == 0 && mLow == 0)
                return "0";

            U128 copy = *this;
            std::string res;
            res.reserve(40);

            // 10^19 — максимальная степень десятки, влезающая в u64.
            // Это позволит нам за один проход получать по 19 цифр.
            constexpr uint64_t div_val = 10000000000000000000ULL;

            while (copy.high() > 0 || copy.low() > 0)
            {
                uint64_t rem;

#if defined(__SIZEOF_INT128__)
                unsigned __int128 val = (static_cast<unsigned __int128>(copy.high()) << 64) | copy.low();
                rem = static_cast<uint64_t>(val % div_val);
                val /= div_val;
                copy.high() = static_cast<uint64_t>(val >> 64);
                copy.low() = static_cast<uint64_t>(val);
#else
                // Быстрая цепочка деления 128/64 для MSVC
                copy.high() = _udiv128(0, copy.high(), div_val, &rem);
                copy.low() = _udiv128(rem, copy.low(), div_val, &rem);
                // Теперь rem содержит остаток от деления всего U128 на 10^19
#endif

                std::string part = std::to_string(rem);

                // Если это не последний блок, дополняем нулями до 19 символов
                if (copy.high() > 0 || copy.low() > 0)
                {
                    res = std::string(19 - part.length(), '0') + part + res;
                }
                else
                {
                    res = part + res;
                }
            }

            return res;
        }

    private:
#if defined(__SIZEOF_INT128__)
        constexpr unsigned __int128 to_u128() const noexcept { return (static_cast<unsigned __int128>(mHigh) << 64) | mLow; }
#endif
    };

static inline U128 mult_ext_manual(u64 x, u64 y) noexcept {
    const u64 x_low  = x & 0xFFFFFFFF;
    const u64 x_high = x >> 32;
    const u64 y_low  = y & 0xFFFFFFFF;
    const u64 y_high = y >> 32;
    const u64 t1  = x_low * y_low;
    const u64 t21 = x_low * y_high;
    const u64 t22 = x_high * y_low;
    const u64 t3  = x_high * y_high;
    const u64 mid = (t1 >> 32) + (t21 & 0xFFFFFFFF) + (t22 & 0xFFFFFFFF);
    return { (t1 & 0xFFFFFFFF) | (mid << 32),
             t3 + (t21 >> 32) + (t22 >> 32) + (mid >> 32) };
}

static inline U128 square_ext_manual(u64 x) noexcept {
    const u64 x_low  = x & 0xFFFFFFFF;
    const u64 x_high = x >> 32;
    const u64 t1 = x_low * x_low;
    const u64 t2 = x_low * x_high;
    const u64 t3 = x_high * x_high;
    const u64 t2_x2 = t2 << 1;
    const u64 t2_carry = t2 >> 63; 
    const u64 mid = (t1 >> 32) + (t2_x2 & 0xFFFFFFFF);
    return { (t1 & 0xFFFFFFFF) | (mid << 32),
             t3 + (mid >> 32) + (t2_x2 >> 32) + (t2_carry << 32) };
}

}

namespace bignum::generic
{
    template <typename U>
    inline constexpr int countl_zero_generic(const U &val)
    {
        if constexpr (requires { val.countl_zero(); })
        {
            // Если у типа есть метод (ваш U128/UBig)
            return val.countl_zero();
        }
        else
        {
            // Если это примитив (uint64_t, uint32_t)
            return std::countl_zero(val);
        }
    }

    /**
     * @brief Универсальное получение остатка от деления.
     */
    template <typename T>
    inline constexpr T get_rem(const T &a, const T &b)
    {
        // 1. Если это наш U128, у которого есть эффективный оператор %
        if constexpr (std::is_same_v<std::decay_t<T>, bignum::u128::U128>)
        {
            return a % b;
        }
        // 2. Если это UBig, который возвращает пару {Q, R}
        else if constexpr (requires { (a / b).second; })
        {
            return (a / b).second;
        }
        // 3. Если это примитив (uint64_t)
        else
        {
            return a % b;
        }
    }

    /**
     * @brief Вычисляет 2^W / x.
     */
    template <class U>
    inline std::pair<U, U> reciprocal_and_extend(U x)
    {
        // Нам нужно, чтобы U поддерживал countl_zero, <<=, - (унарный), / и %
        if (x == U{0})
            return {U{0}, U{0}}; // Или assert

        const auto x_old = x;
        // Использование универсальной функции
        const int i = countl_zero_generic(x);
        x <<= i;
        x = -x; // Теперь x = 2^W - (x_old << i)

        U Q, R;
        if (i > 0)
        {
            // ОПТИМИЗАЦИЯ: используем наш divide, чтобы не делить дважды
            // std::decay_t<U> сделает из "const U128&" просто "U128"
            if constexpr (std::is_same_v<std::decay_t<U>, bignum::u128::U128>)
            {
                // Здесь мы явно вызываем статический метод
                Q = bignum::u128::U128::divide<true, true>(x, x_old, &R);
            }
            else if constexpr (requires { (x / x_old).first; })
            {
                // Дополнительная ветка для UBig (если он возвращает pair)
                auto res = x / x_old;
                Q = res.first;
                R = res.second;
            }
            else
            {
                // Ветка для uint64_t и других примитивов
                Q = x / x_old;
                R = x % x_old;
            }
        }
        else
        {
            // Если i == 0, число x_old очень большое (старший бит 1)
            // x = -x_old эквивалентно (2^W - x_old)
            bool less = (x < x_old);
            Q = less ? U{0} : U{1};
            R = less ? x : U{0}; // Если x >= x_old при i=0, то x_old точно > 2^(W-1)
        }

        Q += (U{1} << i);
        return {Q, R};
    }

    /**
     * @brief r = (r + delta) mod m
     */
    template <class U>
    inline U smart_remainder_adder(U &r, const U &delta, const U &m, const U &r_rec)
    {
        // Используем хелпер для безопасного получения остатка
        const U delta_m = get_rem(delta, m);
        const U summ = r + delta_m;

        const bool overflow = (summ < r);

        // Корректируем сумму
        U next_r = summ + (overflow ? r_rec : U{0});

        // Снова используем хелпер вместо r %= m
        r = get_rem(next_r, m);

        if (overflow)
            return U{1};
        return (summ >= m) ? U{1} : U{0};
    }
}