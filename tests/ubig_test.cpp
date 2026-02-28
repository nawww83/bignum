#include "ubig_test.hpp"
#include "../ubig.hpp"
#include "../u128.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace bignum;
using U128 = bignum::u128::U128;
using U256 = UBig<U128>;
using U512 = UBig<U256>;

#include <random>

// Функция для генерации случайного U128 (вспомогательная)
U128 random_u128(std::mt19937_64& gen) {
    return U128(gen(), gen()); // Собираем из двух 64-битных случайных чисел
}

// Функция для генерации случайного U256
U256 random_u256(std::mt19937_64& gen) {
    return U256(random_u128(gen), random_u128(gen));
}

namespace tests_ubig
{
    void debug_test() 
    {
        {
            U256 x{1};
            U256 y{1};
            auto z = x + y;
            assert(z.toString() == "2");
        } 
        {
            const U256 x = U256::max();
            assert(x.toString() == "115792089237316195423570985008687907853269984665640564039457584007913129639935");
            assert(x.bit_width() == 256);
        }
        {
            const U256 x = U256::max();
            auto y = x * x;
            assert(y.toString() == "1");
        }
        {
            U128 x = U128::max();
            auto y = U256::square_ext(x);
            assert(y.toString() == "115792089237316195423570985008687907852589419931798687112530834793049593217025");
        } 
        //
        {
            U512 x{1};
            U512 y{1};
            auto z = x + y;
            assert(z.toString() == "2");
        } 
        {
            const U512 x = U512::max();
            assert(x.toString() == "13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006084095");
            assert(x.bit_width() == 512);
        } 
    }

void test_u256_division() 
{
    using namespace bignum::u128;
    
    // Определяем U256 на базе вашего U128
    using U256 = UBig<U128>; 

    std::cout << "Starting U256 (UBig<U128>) division tests..." << std::endl;

    // --- Тест 1: Константное деление (Constexpr check) ---
    // Проверяем, что иерархия корректно работает в compile-time
    static constexpr U256 c_a = "200000000000000000000000000000000000000"_u128; // U128 внутри U256
    static constexpr U128 c_b = 2_u128;
    // Если это скомпилировалось, значит вся цепочка constexpr (от u64 до U256) исправна
    constexpr auto c_res = c_a / c_b; 
    static_assert(c_res.first.low() == "100000000000000000000000000000000000000"_u128);

    // --- Тест 2: Деление U256 / U128 (Широкое на Половинку) ---
    // Число > 128 бит делим на число < 128 бит
    {
        // 2^130 + 1000
        U256 a("1361129467683753853853498429727072846824"); 
        U128 b = 4_u128;
        
        auto [q, r] = a / b;
        
        // 2^128 / 4 = 2^128. Здесь q.high должна быть не нулевой
        assert(q.high() > 0_u128);
        assert(q * U256(b) + U256(r) == a);
        std::cout << "Test 2 (U256 / U128) passed." << std::endl;
    }

    // --- Тест 3: Деление U256 / U256 (Широкое на Широкое) ---
    // Проверка нормализации (countl_zero) и циклов коррекции
    {
        // (2^256 - 1) / (2^128)
        U256 a = U256::max();
        U256 b = U256("340282366920938463463374607431768211456");
        
        auto [q, r] = a / b;
        
        // Частное должно быть (2^128 - 1), остаток (2^128 - 1)
        assert( q == r );
        assert(q * b + r == a);
        std::cout << "Test 3 (U256 / U256) passed." << std::endl;
    }

    // --- Тест 4: Авторский итеративный метод (Сложный случай) ---
    // Проверяем сходимость при делителе, близком к границе 128 бит
    {
        U256 a("10000000000000000000000000000000000000000000000000"); 
        U128 b = "340282366920938463463374607431768211454"_u128; // Почти 2^128
        
        auto [q, r] = a / b;
        
        assert(q * U256(b) + U256(r) == a);
        std::cout << "Test 4 (Iterative inverse for U256) passed." << std::endl;
    }

    // --- Тест 5: Деление "в столбик" через границу 128 бит ---
    {
        U128 hi = 10_u128;
        U128 lo = 0_u128;
        U256 a(lo, hi); // 10 * 2^128
        U128 b = 3_u128;
        
        auto [q, r] = a / b;
        
        // 10 / 3 = 3 (в high) и остаток 1 (переходит в low как 2^128 / 3)
        assert(q.high() == 3_u128);
        assert(r == 1_u128); 
        std::cout << "Test 5 (Boundary transition) passed." << std::endl;
    }

    // --- Тест 6: Деление почти равных чисел (Разница в единицу) ---
    {
        U256 a = U256::max();
        U256 b = a - U256(1_u128);
        
        auto [q, r] = a / b;
        
        assert(q == U256(1_u128));
        assert(r == U256(1_u128));
        std::cout << "Test 6 (Almost equal) passed." << std::endl;
    }

    // --- Тест 7: Степенные границы (2^192 / 2^64) ---
    // Проверка, что "прокидывание" остатка между половинками работает корректно
    {
        // 2^192 = high(2^64), low(0)
        U128 hi_val = U128(1_u128) << 64; 
        U256 a(0_u128, hi_val);
        U128 b = U128(1_u128) << 64;
        
        auto [q, r] = a / b;
        
        assert(q == U256(0, 1)); // Ответ должен быть 2^128
        assert(r == 0_u128);
        std::cout << "Test 7 (Power of 2 boundaries) passed." << std::endl;
    }

    // --- Тест 8: "Худший случай" для оценки частного (A*w + B)/(1*w + D) ---
    // Случай, когда оценка Q может ошибиться максимально сильно
    {
        U128 hi = U128::max();
        U128 lo = U128::max();
        U256 a(lo, hi);
        
        // Делитель с большой младшей частью (заставляет оценку Q промахиваться)
        U128 b_val = (U128(1_u128) << 127) | U128::max() >> 1;
        U256 b(b_val);
        
        auto [q, r] = a / b;
        
        assert(q * b + r == a);
        assert(r < b);
        std::cout << "Test 8 (Max error estimation) passed." << std::endl;
    }

    std::cout << "All U256 (Hierarchical UBig) tests passed!" << std::endl;
}

void stress_test_u256(int iterations) 
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    
    std::cout << "Running Stress Test (" << iterations << " iterations)... " << std::endl;

    for (int i = 0; i < iterations; ++i) {
        U256 a = random_u256(gen);
        U256 b = random_u256(gen);

        // Исключаем деление на ноль
        if (b == U256(0ull)) {
            --i; continue; 
        }

        // Выполняем деление
        auto [q, r] = a / b;

        // Основные инварианты деления:
        // 1. a = q * b + r
        if (q * b + r != a) {
            std::cerr << "FAIL: a != q * b + r" << std::endl;
            // Здесь можно вывести значения a, b, q, r в hex для отладки
            assert(false);
        }

        // 2. r < b (остаток всегда строго меньше делителя)
        if (!(r < b)) {
            std::cerr << "FAIL: r >= b" << std::endl;
            assert(false);
        }

        // Периодический отчет о прогрессе
        if (i > 0 && i % 10000 == 0) {
            std::cout << "Passed " << i << " iterations..." << std::endl;
        }
    }
    std::cout << "Stress Test COMPLETED SUCCESSFULLY!" << std::endl;
}


}