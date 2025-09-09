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

        ULOW operator*(const ULOW&) const = delete;
        ULOW& operator*=(const ULOW&) = delete;

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