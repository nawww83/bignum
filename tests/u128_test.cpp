#include <cassert>
#include <random>
#include <iostream>
#include "../defines.h"
#include "../u128.hpp"
#include "../u128_utils.hpp"

using namespace bignum::u128;
using u64 = uint64_t;

namespace
{
    auto const seed = std::random_device{}();

    /***
     * @brief Генератор случайных чисел на отрезке [min_value, max_value].
     * @details Если минимальное значение в смысле беззнакового числа меньше максимального, то минимальное значение интерпретируется как отрицательное число,
     * модуль которого равен этому числу, взятому с отрицательным знаком в смысле оператора "минус" для беззнаковых чисел. Например, переданный интервал [-2ull, 2ull]
     * будет интерпетирован естественно, хотя вместо этого можно было бы передать интервал [18446744073709551614ull, 2ull], что труднее для восприятия.
     * ! Исключение сделано для сочетания (1, 0) - в этом случае диапазон неограничен.
     */
    auto roll_u64 = [urbg = std::mt19937{seed},
                     distr = std::uniform_int_distribution<uint64_t>{}](uint64_t min_value, uint64_t max_value) mutable -> uint64_t
    {
        if (min_value != 1ull && max_value != 0ull)
            return distr(urbg) % (max_value - min_value + 1ull) + min_value;
        return distr(urbg);
    };
}

namespace tests_u128
{
    void debug_test()
    {
        ;
    }

    void string_value_test()
    {
        {
            U128 x{1ull, 1ull};
            const auto &x_str = x.toString();
            assert(x_str == "18446744073709551617");
        }
        {
            U128 x{1ull, 0ull};
            const auto &x_str = x.toString();
            assert(x_str == "1");
        }
        {
            U128 x{0};
            const auto &x_str = x.toString();
            assert(x_str == "0");
        }
        {
            U128 x{123ull, 321ull};
            const auto &x_str = x.toString();
            assert(x_str == "5921404847660766068859");
        }
    }

    void testU128ToString()
    {
        auto check = [](U128 val, const std::string &expected)
        {
            std::string res = val.toString();
            if (res == expected)
            {
                printf("[OK]  %s\n", res.c_str());
            }
            else
            {
                printf("[FAIL] Expected: %s, Got: %s\n", expected.c_str(), res.c_str());
                exit(1);
            }
        };

        printf("--- Тестирование U128::toString() ---\n");

        auto check_hardcoded = [&]() {
            unsigned __int128 raw = 1;
            for(int i = 0; i < 25; ++i) raw *= 10; // Генерируем 10^25 программно

            U128 ten25;
            ten25.low() = (uint64_t)raw;
            ten25.high() = (uint64_t)(raw >> 64);

            std::string res = ten25.toString();
            if (res == "10000000000000000000000000") {
                printf("[OK] Hardcoded 10^25\n");
            } else {
                printf("[FAIL] Hardcoded 10^25: %s\n", res.c_str());
                exit(1);
            }
        };

        check_hardcoded();


        // 1. Базовые случаи
        check({0, 0}, "0");
        check({1, 0}, "1");
        check({999, 0}, "999");

        // 2. Граница 64 бит (2^64)
        // mLow = 0, mHigh = 1
        check({0, 1}, "18446744073709551616");

        // 3. Граница вашего div_val (10^19)
        check({10000000000000000000ULL, 0}, "10000000000000000000");
        check({9999999999999999999ULL, 0}, "9999999999999999999");

        // 4. Случай с "внутренними" нулями в блоках (самый коварный)
        // 10^19 + 1: должно быть '1' и '0000000000000000001'
        U128 ten19_plus_1 = {1, 0}; // Это чуть больше чем 10^19, если mLow=1
        // Чтобы получить ровно 10^19 + 1, нужно:
        U128 t1 = {1, 0};
        t1.low() = 10000000000000000001ULL;
        // Но 10^19 + 1 не влезет в u64, поэтому проверим 10^19 + 7:
        check({10000000000000000007ULL, 0}, "10000000000000000007");

        // 5. Большое число с нулями между блоками
        // Тест: (2 * 10^19) + 5
        // Это число равно 20000000000000000005. 
        // В 128-битном виде это: 
        // high = 1, low = 1553255926290448389 (результат 2e19 - 2^64)
        U128 complex;
        complex.high() = 1; 
        complex.low() = 1553255926290448389ULL; 

        check(complex, "20000000000000000005");
        // Просто возьмем число гарантированно больше 10^19
        U128 big = {0, 2}; // 2 * 2^64 ~ 3.6e19
        check(big, "36893488147419103232");

        // 6. Максимальное значение U128 (2^128 - 1)
        check({0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL},
              "340282366920938463463374607431768211455");

        // 7. 10^25 (ровно 25 нулей)
        U128 ten25;
        ten25.high() = 542101ULL;
        ten25.low() = 1590897978359414784ULL;
        check(ten25, "10000000000000000000000000");

        // 8. 10^38 (почти предел U128)
        U128 ten38;
        ten38.high() = 5421010862427522170ULL;
        ten38.low() = 687399551400673280ULL;
        check(ten38, "100000000000000000000000000000000000000");

        printf("--- Тестирование завершено ---\n");
    }

    void cmp_operator_test()
    {
        {
            U128 x{1, 1};
            U128 y{1, 1};
            assert(x == y);
        }
        {
            U128 x{0, 1};
            U128 y{1, 1};
            assert(x != y);
        }
        {
            U128 x{1, 0};
            U128 y{1, 1};
            assert(x != y);
        }
        {
            U128 x{2, 2};
            U128 y{1, 1};
            assert(x != y);
        }
    }

    void shift_right_operator_test()
    {
        {
            U128 x{0};
            U128 y = x >> 1;
            assert(y == U128{0});
        }
        {
            U128 x{2};
            U128 y = x >> 1;
            assert(y == U128{1});
        }
        {
            U128 x{1};
            U128 y = x >> 1;
            assert(y == U128{0});
        }
        {
            U128 x{1, 1};
            U128 y = x >> 1;
            assert(y == U128{1ull << 63});
        }
        {
            U128 x{1, 1};
            U128 y = x >> 127;
            assert(y == U128{0});
        }
        {
            U128 x{1, 1};
            U128 y = x >> 128;
            assert(y == U128{0});
        }
    }

    void shift_left_operator_test()
    {
        {
            U128 x{0};
            U128 y = x << 1;
            assert(y == U128{0});
        }
        {
            U128 x{1};
            U128 y = x << 1;
            assert(y == U128{2});
        }
        {
            U128 x{1, 1};
            U128 y = x << 1;
            assert((y == U128{2, 2}));
        }
        {
            U128 x{1};
            U128 y = x << 127;
            assert((y == U128{0, 1ull << 63}));
        }
        {
            U128 x{1, 1};
            U128 y = x << 128;
            assert(y == U128{0});
        }
    }

    void and_operator_test()
    {
        U128 x{1, 1};
        U128 y{0, 1};
        U128 z = x & y;
        assert((z == U128{0, 1}));
    }

    void or_operator_test()
    {
        U128 x{0, 1};
        U128 y{1, 1};
        U128 z = x | y;
        assert((z == U128{1, 1}));
    }

    void xor_operator_test()
    {
        U128 x{1, 1};
        U128 y{1, 1};
        U128 z = x ^ y;
        assert(z == U128{0});
    }

    void addition_test()
    {
        {
            U128 x{1, 1};
            U128 y{2, 2};
            U128 z = x + y;
            assert((z == U128{3, 3}));
        }
        {
            U128 x{1ull << 63};
            U128 y{1ull << 63};
            U128 z = x + y;
            assert((z == U128{0, 1}));
        }
    }

    void subtraction_test()
    {
        {
            U128 x{2, 2};
            U128 y{1, 1};
            U128 z = x - y;
            assert((z == U128{1, 1}));
        }
        {
            U128 x{0};
            U128 y{1};
            U128 z = x - y;
            assert((z == U128::max()));
        }
    }

    void multiplication_test()
    {
        {
            U128 x{1, 2};                    // 36893488147419103233
            U128 y{2, 1};                    // 18446744073709551618
            U128 z = x * y;                  // 680564733841876927018982935232084180994 mod 2^128 = 92233720368547758082
            assert((z == U128{2ull, 5ull})); // 92233720368547758082 = 2 + 5 * 2^64
        }
        {
            U128 x{0, 1ull << 63};
            U128 y{3};
            U128 z = x * y; // = xy mod 2^128.
            assert((z == x));
        }
        {
            U128 x{2};
            U128 z = x * x;
            assert(z == U128{4});
        }
    }

    void mult_ext_test()
    {

        // --- Автоматический тестер ручного алгоритма умножения чисел с расширением разрядности ---
        auto run_test = [](u64 x, u64 y, const char *label, bool show = true)
        {
            // Вычисляем эталон через встроенный тип __int128
            unsigned __int128 reference = static_cast<unsigned __int128>(x) * y;
            u64 exp_h = static_cast<u64>(reference >> 64);
            u64 exp_l = static_cast<u64>(reference);

            U128 result = (x == y) ? square_ext_manual(x) : mult_ext_manual(x, y);

            if (result.high() == exp_h && result.low() == exp_l)
            {
                if (show)
                    std::cout << "[OK]   " << label << " (x=" << std::hex << x << ", y=" << y << ")\n";
            }
            else
            {
                std::cout << "[FAIL] " << label << "\n";
                std::cout << std::hex << "  Input:    x=" << x << ", y=" << y << "\n";
                std::cout << "  Expected: {" << exp_h << ", " << exp_l << "}\n";
                std::cout << "  Got:      {" << result.high() << ", " << result.low() << "}\n"
                          << std::dec;
                exit(1); // Прекращаем выполнение при первой ошибке
            }
        };

        std::cout << "--- Starting Automated Tests with __int128 reference ---\n\n";

        // Граничные кейсы
        run_test(0, 0, "Zeroes");
        run_test(0xFFFFFFFFFFFFFFFFULL, 1, "Max * 1");
        run_test(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, "Max * Max");
        run_test(0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, "Pattern A Square");
        run_test(0x5555555555555555ULL, 0x5555555555555555ULL, "Pattern 5 Square");
        run_test(1ULL << 32, 1ULL << 32, "Boundary 2^32 Square");
        run_test(1ULL << 63, 2, "High-bit carry mult");

        // Небольшой стресс-тест на псевдослучайных числах
        u64 seed_x = 0x123456789ABCDEF0ULL;
        u64 seed_y = 0xFEDCBA9876543210ULL;
        for (int i = 0; i < 1000; ++i)
        {
            seed_x ^= seed_x << 13;
            seed_x ^= seed_x >> 7;
            seed_x ^= seed_x << 17; // xorshift
            seed_y ^= seed_y << 13;
            seed_y ^= seed_y >> 7;
            seed_y ^= seed_y << 17;
            run_test(seed_x, seed_y, "Random Mult", false);
            run_test(seed_x, seed_x, "Random Square", false);
        }

        std::cout << "\n"
                  << std::dec << "ALL TESTS (including some random cases) PASSED!\n";
    }

    void division_test()
    {
        // Половинчатое деление.
        {
            U128 x{8};
            u64 y{2};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{4ull});
            assert(r == U128{0});
        }
        {
            U128 x{4536ull, 443ull};
            u64 y{132668453ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{61596464267608ull});
            assert(r == U128{0});
        }
        {
            U128 x{5ull, 7ull};
            u64 y{1ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == x);
            assert(r == U128{0});
        }
        // Деление.
        {
            U128 x{0};
            U128 y{4};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{0});
            assert(r == U128{0});
        }
        {
            U128 x{1};
            U128 y{1};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{1ull});
            assert(r == U128{0});
        }
        {
            U128 x{8};
            U128 y{2};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{4ull});
            assert(r == U128{0});
        }
        {
            U128 x{3};
            U128 y{5};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{0});
            assert(r == U128{3ull});
        }
        {
            U128 x{2, 2};
            U128 y{2};
            auto q = x / y;
            auto r = x % y;
            assert((q == U128{1ull, 1ull}));
            assert(r == U128{0});
        }
        {
            U128 x{4536ull, 443ull};
            U128 y{132668453ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{61596464267608ull});
            assert(r == U128{0});
        }
        {
            U128 x{4536ull, 443ull};
            U128 y{5ull, 3ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{147ull});
            assert((r == U128{3801ull, 2ull}));
        }
        {
            U128 x{4536ull, 443ull};
            U128 y{5ull, 1ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{443ull});
            assert(r == U128{2321ull});
        }
        {
            U128 x{0, 1ull};
            U128 y{2ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{1ull << 63});
            assert(r == U128{0});
        }
        {
            U128 x{0, 1ull};
            U128 y{13ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{1418980313362273201ull});
            assert(r == U128{3ull});
        }
        {
            U128 x{112ull, 1ull};
            U128 y{13ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{1418980313362273209ull});
            assert(r == U128{11ull});
        }
        {
            U128 x{2ull, 3ull};
            U128 y{3ull, 1ull};
            auto q = x / y;
            auto r = x % y;
            assert(q == U128{2ull});
            assert(r == U128{18446744073709551612ull});
        }
        {
            // 113343289537830031080300281241835621701 = 287606173964874511799394197485154948483 mod 174262884427044480719093916243319326782
            U128 x{7570750807943894403ull, 15591161931648043505ull};
            U128 y{7851060955248855102ull, 9446809893969600230ull};
            auto r = x % y;
            assert((r == U128{18166433926404590917ull, 6144352037678443274ull}));
        }
    }

    void reciprocal_test()
    {
        using namespace bignum::generic;
        {
            auto x = u64{1};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == 0 && r == 0));
        }
        {
            auto x = u64{2};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == u64{1ull << 63} && r == 0));
        }
        {
            auto x = u64{3};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == 6148914691236517205ull && r == 1));
        }
        {
            auto x = u64{-1ull};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == 1 && r == 1));
        }
        {
            auto x = u64{-2ull};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == 1 && r == 2));
        }
    }

    void bit_length_test()
    {
        {
            U128 x{0};
            assert(x.bit_width() == 0);
        }
        {
            U128 x{1};
            assert(x.bit_width() == 1);
        }
        {
            U128 x{2};
            assert(x.bit_width() == 2);
        }
        {
            U128 x{127};
            assert(x.bit_width() == 7);
        }
        {
            U128 x{128};
            assert(x.bit_width() == 8);
        }
        {
            U128 x = U128::max();
            assert(x.bit_width() == 128);
        }
    }

    void sqrt_test()
    {
        using namespace u128_utils;
        {
            U128 x{1};
            U128 y = isqrt(x);
            assert(y.toString() == "1");
        }
        {
            U128 x{4};
            bool exact;
            U128 y = isqrt(x, exact);
            assert(y.toString() == "2");
            assert(exact);
        }
        {
            U128 x{5};
            bool exact;
            U128 y = isqrt(x, exact);
            assert(y.toString() == "2");
            assert(!exact);
        }
        {
            U128 x = U128::max();
            U128 y = isqrt(x);
            assert(y.toString() == "18446744073709551615");
        }
    }

    void nroot_test()
    {
        using namespace u128_utils;
        {
            U128 x = U128::max();
            U128 y = nroot(x, 1);
            assert(y == x);
        }
        {
            U128 x = U128::max();
            U128 y = nroot(x, 2);
            assert(y.toString() == "18446744073709551615");
        }
        {
            U128 x = U128::max();
            U128 y = nroot(x, 3);
            assert(y.toString() == "6981463658331");
        }
        {
            U128 x = U128::max();
            U128 y = nroot(x, 4);
            assert(y.toString() == "4294967295");
        }
        {
            U128 x = U128::max();
            U128 y = nroot(x, 5);
            assert(y.toString() == "50859008");
        }
        // 5480386857784802185939 = 19^17
        {
            U128 x{1703867893065355987ull, 297ull};
            U128 y = nroot(x, 17);
            assert(y.toString() == "19");
        }
        {
            U128 x{8071991882386131446ull, 5373886718148709326ull};
            U128 y = nroot(x, 17);
            assert(y.toString() == "171");
        }
    }

    void mult_mod_test()
    {
        using namespace u128_utils;
        {
            U128 x{1, 1};
            U128 y{1, 1};
            U128 m = U128::max();
            U128 z = mult_mod(x, y, m);
            assert(z.toString() == "36893488147419103234");
        }
        {
            U128 x{1, 1};
            U128 y{1, 1};
            U128 m{1, 1};
            U128 z = mult_mod(x, y, m);
            assert(z == U128{0});
        }
        {
            U128 x{3, 1};
            U128 y{1, 1};
            U128 m{1, 1};
            U128 z = mult_mod(x, y, m);
            assert(z == U128{0});
        }
        {
            U128 x{11, 5};
            U128 y{3, 7};
            U128 m{17, 13};
            U128 z = mult_mod(x, y, m);
            assert(z.toString() == "151830893529763232515");
        }
    }

    void modular_inverse_test()
    {
        using namespace u128_utils;
        {
            U128 x{2};
            U128 m{13};
            bool ok;
            U128 y = modular_inverse(x, m, ok);
            assert(ok);
            assert(y.toString() == "7");
        }
        {
            U128 x{2};
            U128 m{4};
            bool ok;
            U128 y = modular_inverse(x, m, ok);
            assert(!ok);
        }
        {
            // 53118967482164732838376890042435232507, 91146857343605793793601199626440586221;
            U128 x{7061992801194730235ull, 2879584997217493442ull};
            U128 m{8275130208243856365ull, 4941081037358187766ull};
            bool ok;
            U128 y = modular_inverse(x, m, ok);
            assert(ok);
            assert(y.toString() == "31343560153576310990713916189340233887");
        }
    }

    void quadratic_residue_test()
    {
        using namespace u128_utils;
        const U128 x{15347ull};
        {
            const U128 p{2ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(is_ok);
        }
        {
            const U128 p{17ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(is_ok);
        }
        {
            const U128 p{23ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(is_ok);
        }
        {
            const U128 p{29ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(is_ok);
        }
        {
            const U128 p{31ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(is_ok);
        }
        {
            const U128 p{3ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(!is_ok);
        }
        {
            const U128 p{5ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(!is_ok);
        }
        {
            const U128 p{7ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(!is_ok);
        }
        {
            const U128 p{11ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(!is_ok);
        }
        {
            const U128 p{13ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(!is_ok);
        }
        {
            const U128 p{19ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(!is_ok);
        }
        {
            const U128 p{37ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(!is_ok);
        }
        {
            const U128 p{41ull};
            bool is_ok = is_quadratic_residue(x, p);
            assert(!is_ok);
        }
    }

    void sqrt_mod_test()
    {
        using namespace u128_utils;
        {
            const U128 x{3435ull};
            const U128 p{13ull};
            auto [x1, x2] = sqrt_mod(x, p);
            assert(x1 == U128{4ull});
            assert(x2 == U128{9ull});
        }
        {
            const U128 x{26ull};
            const U128 p{13ull};
            auto [x1, x2] = sqrt_mod(x, p);
            assert(x1 == U128{0ull});
            assert(x2 == U128{0ull});
        }
    }

    void div_mod_test()
    {
        using namespace u128_utils;
        {
            const U128 x{35ull};
            const U128 y{3ull};
            const U128 p{13ull};
            auto q = div_mod(x, y, p);
            assert(((q * y) % p) == (x % p));
        }
        {
            const U128 x{35ull};
            const U128 y{7ull};
            const U128 p{13ull};
            auto q = div_mod(x, y, p);
            assert(((q * y) % p) == (x % p));
        }
        {
            const U128 x{14ull};
            const U128 y{8ull};
            const U128 p{7ull};
            auto q = div_mod(x, y, p);
            assert(((q * y) % p) == (x % p));
        }
        {
            const U128 x{0ull};
            const U128 y{8ull};
            const U128 p{8ull};
            auto q = div_mod(x, y, p);
            assert(((q * y) % p) == (x % p));
        }
        {
            const U128 x{16ull};
            const U128 y{8ull};
            const U128 p{8ull};
            auto q = div_mod(x, y, p);
            assert(((q * y) % p) == (x % p));
        }
    }

    void random_half_division_test(uint64_t min_value, uint64_t max_value, int num_of_parts, size_t number_of_iterations_per_part)
    {
        std::cout << "Run half-division random test";
        if (min_value != 1 && max_value != 0)
            std::cout << ": [" << static_cast<int64_t>(min_value) << "..." << max_value << "]\n";
        else
            std::cout << ": any values." << std::endl;
        std::cout << "...\n"
                  << std::flush;
        uint64_t counter = 0;
        int part_counter = 0;
        for (;;)
        {
            counter++;
            const U128 x{roll_u64(min_value, max_value), roll_u64(min_value, max_value)};
            const u64 y{roll_u64(min_value, max_value)};
            if (y == u64{0})
                continue;
            const auto q = x / y;
            const auto r = x % y;
            const auto &x_restored = q * y + r;
            const bool is_rem_ok = r < U128{y};
            const bool equality = x_restored == x;
            if (!is_rem_ok || !equality)
            {
                std::cout << "x: " << x.toString() << std::endl;
                std::cout << "y: " << y << std::endl;
            }
            if ((counter % number_of_iterations_per_part) == 0)
            {
                part_counter++;
                std::cout << "ok: counter: " << counter << ", part " << part_counter << " from: " << num_of_parts << std::endl;
                // std::cout << "\tlast x // y = " << x.toString() << " // " << y() << " = " << q.toString() << ", remainder = " << r.toString();
                // std::cout << std::endl
                //   << std::flush;
            }
            assert(is_rem_ok);
            assert(equality);
            if (part_counter >= num_of_parts)
                break;
        }
        std::cout << "Random test finished. Ok!\n\n";
    }

    void random_full_division_test(uint64_t min_value, uint64_t max_value, int num_of_parts, size_t number_of_iterations_per_part)
    {
        std::cout << "Run full division random test";
        if (min_value != 1 && max_value != 0)
            std::cout << ": [" << static_cast<int64_t>(min_value) << "..." << max_value << "]\n";
        else
            std::cout << ": any values." << std::endl;
        std::cout << "...\n"
                  << std::flush;
        uint64_t counter = 0;
        int part_counter = 0;
        for (;;)
        {
            counter++;
            const U128 x{roll_u64(min_value, max_value), roll_u64(min_value, max_value)};
            const U128 y{roll_u64(min_value, max_value), roll_u64(min_value, max_value)};
            if (y == U128{0})
                continue;
            const auto q = x / y;
            const auto r = x % y;
            const auto &x_restored = q * y + r;
            const bool is_rem_ok = r < y;
            const bool equality = x_restored == x;
            if (!is_rem_ok || !equality)
            {
                std::cout << "x: " << x.toString() << std::endl;
                std::cout << "y: " << y.toString() << std::endl;
            }
            if ((counter % number_of_iterations_per_part) == 0)
            {
                part_counter++;
                std::cout << "ok: counter: " << counter << ", part " << part_counter << " from: " << num_of_parts << std::endl;
                // std::cout << "\tlast x // y = " << x.toString() << " // " << y.toString() << " = " << q.toString() << ", remainder = " << r.toString();
                // std::cout << std::endl
                //   << std::flush;
            }
            assert(is_rem_ok);
            assert(equality);
            if (part_counter >= num_of_parts)
                break;
        }
        std::cout << "Random test finished. Ok!\n\n";
    }
}