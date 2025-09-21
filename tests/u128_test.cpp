#include <cassert>
#include <random>
#include <iostream>
#include "../defines.h"
#include "../u128.hpp"

using namespace bignum::u128;

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
    auto roll_ulow = [urbg = std::mt19937{seed},
                      distr = std::uniform_int_distribution<u64>{}](uint64_t min_value, uint64_t max_value) mutable -> u64
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
            const auto &x_str = x.value();
            assert(x_str == "18446744073709551617");
        }
        {
            U128 x{1ull, 0ull};
            const auto &x_str = x.value();
            assert(x_str == "1");
        }
        {
            U128 x{0};
            const auto &x_str = x.value();
            assert(x_str == "0");
        }
        {
            U128 x{123ull, 321ull};
            const auto &x_str = x.value();
            assert(x_str == "5921404847660766068859");
        }
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
            assert((z == U128::get_max_value()));
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
            U128 x{2};
            U128 z = x * x;
            assert(z == U128{4});
        }
    }

    int division_test()
    {
        int num_of_runned_tests = 0;
        // Половинчатое деление.
        {
            num_of_runned_tests++;
            U128 x{8};
            ULOW y{2};
            auto [q, r] = x / y;
            assert(q == U128{4ull});
            assert(r == U128{0});
        }
        {
            num_of_runned_tests++;
            U128 x{4536ull, 443ull};
            ULOW y{132668453ull};
            auto [q, r] = x / y;
            assert(q == U128{61596464267608ull});
            assert(r == U128{0});
        }
        {
            num_of_runned_tests++;
            U128 x{5ull, 7ull};
            ULOW y{1ull};
            auto [q, r] = x / y;
            assert(q == x);
            assert(r == U128{0});
        }
        // Деление.
        {
            num_of_runned_tests++;
            U128 x{0};
            U128 y{4};
            auto [q, r] = x / y;
            assert(q == U128{0});
            assert(r == U128{0});
        }
        {
            num_of_runned_tests++;
            U128 x{1};
            U128 y{1};
            auto [q, r] = x / y;
            assert(q == U128{1ull});
            assert(r == U128{0});
        }
        {
            num_of_runned_tests++;
            U128 x{8};
            U128 y{2};
            auto [q, r] = x / y;
            assert(q == U128{4ull});
            assert(r == U128{0});
        }
        {
            num_of_runned_tests++;
            U128 x{3};
            U128 y{5};
            auto [q, r] = x / y;
            assert(q == U128{0});
            assert(r == U128{3ull});
        }
        {
            num_of_runned_tests++;
            U128 x{2, 2};
            U128 y{2};
            auto [q, r] = x / y;
            assert((q == U128{1ull, 1ull}));
            assert(r == U128{0});
        }
        {
            num_of_runned_tests++;
            U128 x{4536ull, 443ull};
            U128 y{132668453ull};
            auto [q, r] = x / y;
            assert(q == U128{61596464267608ull});
            assert(r == U128{0});
        }
        {
            num_of_runned_tests++;
            U128 x{4536ull, 443ull};
            U128 y{5ull, 3ull};
            auto [q, r] = x / y;
            assert(q == U128{147ull});
            assert((r == U128{3801ull, 2ull}));
        }
        {
            num_of_runned_tests++;
            U128 x{4536ull, 443ull};
            U128 y{5ull, 1ull};
            auto [q, r] = x / y;
            assert(q == U128{443ull});
            assert(r == U128{2321ull});
        }
        {
            num_of_runned_tests++;
            U128 x{0, 1ull};
            U128 y{2ull};
            auto [q, r] = x / y;
            assert(q == U128{1ull << 63});
            assert(r == U128{0});
        }
        {
            num_of_runned_tests++;
            U128 x{0, 1ull};
            U128 y{13ull};
            auto [q, r] = x / y;
            assert(q == U128{1418980313362273201ull});
            assert(r == U128{3ull});
        }
        {
            num_of_runned_tests++;
            U128 x{112ull, 1ull};
            U128 y{13ull};
            auto [q, r] = x / y;
            assert(q == U128{1418980313362273209ull});
            assert(r == U128{11ull});
        }
        {
            num_of_runned_tests++;
            U128 x{2ull, 3ull};
            U128 y{3ull, 1ull};
            auto [q, r] = x / y;
            assert(q == U128{2ull});
            assert(r == U128{18446744073709551612ull});
        }
        return num_of_runned_tests;
    }

    void reciprocal_test()
    {
        {
            auto x = ULOW{1};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == 0 && r == 0));
        }
        {
            auto x = ULOW{2};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == ULOW{1ull << 63} && r == 0));
        }
        {
            auto x = ULOW{3};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == 6148914691236517205ull && r == 1));
        }
        {
            auto x = ULOW{-1ull};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == 1 && r == 1));
        }
        {
            auto x = ULOW{-2ull};
            auto [q, r] = reciprocal_and_extend(x);
            assert((q == 1 && r == 2));
        }
    }

    void random_half_division_test(uint64_t min_value, uint64_t max_value, int num_of_parts, size_t number_of_iterations_per_part)
    {
#ifdef USE_DIV_COUNTERS
        g_all_half_divs = 0;
        g_average_loops_when_half_div = 0;
        g_max_loops_when_half_div = 0;
        g_min_loops_when_half_div = 128;
        for (size_t i = 0; i < 128; i++)
        {
            g_hist[i] = 0;
        }
#endif
        std::cout << "Run half-division random test";
        if (min_value != 1 && max_value != 0)
            std::cout << ": [" << static_cast<int64_t>(min_value) << "..." << max_value << "]\n";
        else
            std::cout << ": any values." << std::endl;
        std::cout << "..." << std::flush;
        uint64_t counter = 0;
        int part_counter = 0;
        for (;;)
        {
            counter++;
            const U128 x{roll_ulow(min_value, max_value), roll_ulow(min_value, max_value)};
            const ULOW y{roll_ulow(min_value, max_value)};
            if (y == ULOW{0})
                continue;
            const auto &[q, r] = x / y;
            const auto &x_restored = q * y + r;
            const bool is_rem_ok = r < U128{y};
            const bool equality = x_restored == x;
            if (!is_rem_ok || !equality)
            {
                std::cout << "x: " << x.value() << std::endl;
                std::cout << "y: " << y() << std::endl;
            }
            if ((counter % number_of_iterations_per_part) == 0)
            {
                part_counter++;
                std::cout << "ok: counter: " << counter << ", part " << part_counter << " from: " << num_of_parts << std::endl;
#ifdef USE_DIV_COUNTERS
                std::cout << " loops per division: ave: " << g_average_loops_when_half_div << ", min: " << g_min_loops_when_half_div << ", max: " << g_max_loops_when_half_div << ";\n";
#endif
// std::cout << "\tlast x // y = " << x.value() << " // " << y() << " = " << q.value() << ", remainder = " << r.value();
// std::cout << std::endl
//   << std::flush;
#ifdef USE_DIV_COUNTERS
                std::cout << "\thist: ";
                double sum = 0;
                for (size_t i = 0; i < 128; i++)
                {
                    sum += g_hist[i];
                }
                bool was_opened = false;
                bool was_closed = false;
                for (size_t i = 0; i < 128; i++)
                {
                    if (i > 0 && was_opened)
                    {
                        std::cout << ")";
                        was_closed = true;
                        was_opened = false;
                    }
                    if (g_hist[i])
                    {
                        std::cout << (was_closed ? ", " : "") << "(" << i << " : " << static_cast<float>(g_hist[i] / sum);
                        was_opened = true;
                        was_closed = false;
                    }
                }
                std::cout << ".";
                std::cout << std::endl
                          << std::flush;
#endif
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
#ifdef USE_DIV_COUNTERS
        g_all_half_divs = 0;
        g_average_loops_when_half_div = 0;
        g_max_loops_when_half_div = 0;
        g_min_loops_when_half_div = 0;
        g_all_divs = 0;
        g_average_loops_when_div = 0;
        g_max_loops_when_div = 0;
        g_min_loops_when_div = 128;
        for (size_t i = 0; i < 128; i++)
        {
            g_hist[i] = 0;
        }
#endif
        std::cout << "Run full division random test";
        if (min_value != 1 && max_value != 0)
            std::cout << ": [" << static_cast<int64_t>(min_value) << "..." << max_value << "]\n";
        else
            std::cout << ": any values." << std::endl;
        std::cout << "..." << std::flush;
        uint64_t counter = 0;
        int part_counter = 0;
        for (;;)
        {
            counter++;
            const U128 x{roll_ulow(min_value, max_value), roll_ulow(min_value, max_value)};
            const U128 y{roll_ulow(min_value, max_value), roll_ulow(min_value, max_value)};
            if (y == U128{0})
                continue;
            const auto &[q, r] = x / y;
            const auto &x_restored = q * y + r;
            const bool is_rem_ok = r < y;
            const bool equality = x_restored == x;
            if (!is_rem_ok || !equality)
            {
                std::cout << "x: " << x.value() << std::endl;
                std::cout << "y: " << y.value() << std::endl;
            }
            if ((counter % number_of_iterations_per_part) == 0)
            {
                part_counter++;
                std::cout << "ok: counter: " << counter << ", part " << part_counter << " from: " << num_of_parts << std::endl;
#ifdef USE_DIV_COUNTERS
                std::cout << " loops per division: full div: ave: " << g_average_loops_when_div << ", min: " << g_min_loops_when_div << ", max: " << g_max_loops_when_div << ", half div: ave: " << g_average_loops_when_half_div << ", min: " << g_min_loops_when_half_div << ", max: " << g_max_loops_when_half_div << ";\n";
#endif
// std::cout << "\tlast x // y = " << x.value() << " // " << y.value() << " = " << q.value() << ", remainder = " << r.value();
// std::cout << std::endl
//   << std::flush;
#ifdef USE_DIV_COUNTERS
                std::cout << "\thist: ";
                double sum = 0;
                for (size_t i = 0; i < 128; i++)
                {
                    sum += g_hist[i];
                }
                bool was_opened = false;
                bool was_closed = false;
                for (size_t i = 0; i < 128; i++)
                {
                    if (i > 0 && was_opened)
                    {
                        std::cout << ")";
                        was_closed = true;
                        was_opened = false;
                    }
                    if (g_hist[i])
                    {
                        std::cout << (was_closed ? ", " : "") << "(" << i << " : " << static_cast<float>(g_hist[i] / sum);
                        was_opened = true;
                        was_closed = false;
                    }
                }
                std::cout << ".";
                std::cout << std::endl
                          << std::flush;
#endif
            }
            assert(is_rem_ok);
            assert(equality);
            if (part_counter >= num_of_parts)
                break;
        }
        std::cout << "Random test finished. Ok!\n\n";
    }
}