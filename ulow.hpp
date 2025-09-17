#pragma once

#include <cstdint> // uint64_t

namespace low64
{
    /**
     * @brief Класс для хранения числа-половинки. В данном случае - 64-битное беззнаковое число.
     * @details 
     *  - Позволяет иметь общее имя ULOW вне зависимости от разрядности числа-половинки.
     *  - Позволяет открыть оператор умножения (ULOW * Полное число), когда число-половинка находится слева полного числа.
     */
    class ULOW
    {
    public:
        /**
         * @brief Конструктор по умолчанию.
         */
        explicit constexpr ULOW() = default;
        
        /**
         * @brief Конструктор с параметром.
         */
        constexpr ULOW(uint64_t value) : mValue(value) {}

        /**
         * @brief
         */
        auto operator<=>(const ULOW&) const = default; 

        /**
         * @brief Оператор доступа к числу.
         */
        uint64_t operator()() const
        {
            return mValue;
        }

        /**
         * @brief Оператор доступа к числу.
         */
        constexpr uint64_t &operator()()
        {
            return mValue;
        }

        /**
         * @brief Оператор инверсии битов.
         */
        ULOW operator~() const
        {
            ULOW result = *this;
            result.mValue = ~result.mValue;
            return result;
        }

        /**
         * @brief Оператор минус.
         */
        ULOW operator-() const
        {
            ULOW result = *this;
            result.mValue = -result.mValue;
            return result;
        }

        ULOW operator<<(uint32_t shift) const
        {
            ULOW result = *this;
            result.mValue <<= shift;
            return result;
        }

        ULOW& operator<<=(uint32_t shift)
        {
            *this = *this << shift;
            return *this;
        }

        ULOW operator>>(uint32_t shift) const
        {
            ULOW result = *this;
            result.mValue >>= shift;
            return result;
        }

        ULOW& operator>>=(uint32_t shift)
        {
            *this = *this >> shift;
            return *this;
        }

        ULOW operator&(const ULOW& rhs) const {
            ULOW result = *this;
            result.mValue &= rhs.mValue;
            return result;
        }

        ULOW& operator&=(const ULOW& rhs) {
            *this = *this & rhs;
            return *this;
        }

        ULOW operator|(const ULOW& rhs) const {
            ULOW result = *this;
            result.mValue |= rhs.mValue;
            return result;
        }

        ULOW& operator|=(const ULOW& rhs) {
            *this = *this | rhs;
            return *this;
        }

        ULOW operator^(const ULOW& rhs) const {
            ULOW result = *this;
            result.mValue ^= rhs.mValue;
            return result;
        }

        ULOW& operator^=(const ULOW& rhs) {
            *this = *this ^ rhs;
            return *this;
        }

        ULOW operator+(const ULOW& rhs) const {
            ULOW result = *this;
            result.mValue += rhs.mValue;
            return result;
        }

        ULOW& operator+=(const ULOW& rhs) {
            *this = *this + rhs;
            return *this;
        }

        ULOW operator-(const ULOW& rhs) const {
            ULOW result = *this;
            result.mValue -= rhs.mValue;
            return result;
        }

        ULOW& operator-=(const ULOW& rhs) {
            *this = *this - rhs;
            return *this;
        }

        ULOW operator*(const ULOW& rhs) const {
            ULOW result = *this;
            result.mValue *= rhs.mValue;
            return result;
        }

        ULOW& operator*=(const ULOW& rhs) {
            *this = *this * rhs;
            return *this;
        }

        ULOW operator/(const ULOW& rhs) const {
            ULOW result = *this;
            result.mValue /= rhs.mValue;
            return result;
        }

        ULOW& operator/=(const ULOW& rhs) {
            *this = *this / rhs;
            return *this;
        }

        ULOW operator%(const ULOW& rhs) const {
            ULOW result = *this;
            result.mValue %= rhs.mValue;
            return result;
        }

        ULOW& operator%=(const ULOW& rhs) {
            *this = *this % rhs;
            return *this;
        }

        /**
         * @brief Оператор умножения. Позволяет перемножать половинчатые числа, расположенные слева от полного числа.
         */
        template <typename T>
        T operator*(const T &rhs) const
        {
            T result = rhs * *this;
            return result;
        }

        /**
         * @brief
         */
        template <typename T>
        T &operator*=(const T &) = delete;

    private:
        /**
         * @brief Непосредственно число.
         */
        uint64_t mValue{0};
    };
}