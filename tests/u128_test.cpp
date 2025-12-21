#include <cassert>
#include <random>
#include <iostream>
#include "../defines.h"
#include "../u128.hpp"
#include "../u128_utils.hpp"

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

    void division_test()
    {
        // Половинчатое деление.
        {
            U128 x{8};
            ULOW y{2};
            auto [q, r] = x / y;
            assert(q == U128{4ull});
            assert(r == U128{0});
        }
        {
            U128 x{4536ull, 443ull};
            ULOW y{132668453ull};
            auto [q, r] = x / y;
            assert(q == U128{61596464267608ull});
            assert(r == U128{0});
        }
        {
            U128 x{5ull, 7ull};
            ULOW y{1ull};
            auto [q, r] = x / y;
            assert(q == x);
            assert(r == U128{0});
        }
        // Деление.
        {
            U128 x{0};
            U128 y{4};
            auto [q, r] = x / y;
            assert(q == U128{0});
            assert(r == U128{0});
        }
        {
            U128 x{1};
            U128 y{1};
            auto [q, r] = x / y;
            assert(q == U128{1ull});
            assert(r == U128{0});
        }
        {
            U128 x{8};
            U128 y{2};
            auto [q, r] = x / y;
            assert(q == U128{4ull});
            assert(r == U128{0});
        }
        {
            U128 x{3};
            U128 y{5};
            auto [q, r] = x / y;
            assert(q == U128{0});
            assert(r == U128{3ull});
        }
        {
            U128 x{2, 2};
            U128 y{2};
            auto [q, r] = x / y;
            assert((q == U128{1ull, 1ull}));
            assert(r == U128{0});
        }
        {
            U128 x{4536ull, 443ull};
            U128 y{132668453ull};
            auto [q, r] = x / y;
            assert(q == U128{61596464267608ull});
            assert(r == U128{0});
        }
        {
            U128 x{4536ull, 443ull};
            U128 y{5ull, 3ull};
            auto [q, r] = x / y;
            assert(q == U128{147ull});
            assert((r == U128{3801ull, 2ull}));
        }
        {
            U128 x{4536ull, 443ull};
            U128 y{5ull, 1ull};
            auto [q, r] = x / y;
            assert(q == U128{443ull});
            assert(r == U128{2321ull});
        }
        {
            U128 x{0, 1ull};
            U128 y{2ull};
            auto [q, r] = x / y;
            assert(q == U128{1ull << 63});
            assert(r == U128{0});
        }
        {
            U128 x{0, 1ull};
            U128 y{13ull};
            auto [q, r] = x / y;
            assert(q == U128{1418980313362273201ull});
            assert(r == U128{3ull});
        }
        {
            U128 x{112ull, 1ull};
            U128 y{13ull};
            auto [q, r] = x / y;
            assert(q == U128{1418980313362273209ull});
            assert(r == U128{11ull});
        }
        {
            U128 x{2ull, 3ull};
            U128 y{3ull, 1ull};
            auto [q, r] = x / y;
            assert(q == U128{2ull});
            assert(r == U128{18446744073709551612ull});
        }
        {
            // 113343289537830031080300281241835621701 = 287606173964874511799394197485154948483 mod 174262884427044480719093916243319326782
            U128 x{7570750807943894403ull, 15591161931648043505ull};
            U128 y{7851060955248855102ull, 9446809893969600230ull};
            auto [_, r] = x / y;
            assert((r == U128{18166433926404590917ull, 6144352037678443274ull}));
        }
    }

    void reciprocal_test()
    {
        using namespace bignum::generic;
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

    void bit_length_test()
    {
        {
            U128 x{0};
            assert(x.bit_length() == 0);
        }
        {
            U128 x{1};
            assert(x.bit_length() == 1);
        }
        {
            U128 x{2};
            assert(x.bit_length() == 2);
        }
        {
            U128 x{127};
            assert(x.bit_length() == 7);
        }
        {
            U128 x{128};
            assert(x.bit_length() == 8);
        }
        {
            U128 x = U128::get_max_value();
            assert(x.bit_length() == 128);
        }
    }

    void sqrt_test()
    {
        using namespace u128_utils;
        {
            U128 x = U128::get_max_value();
            U128 y = isqrt(x);
            assert(y.value() == "18446744073709551615");
        }
    }

    void mult_mod_test()
    {
        using namespace u128_utils;
        {
            U128 x {1, 1};
            U128 y {1, 1};
            U128 m = U128::get_max_value();
            U128 z = mult_mod(x, y, m);
            assert(z.value() == "36893488147419103234");
        }
        {
            U128 x {1, 1};
            U128 y {1, 1};
            U128 m {1, 1};
            U128 z = mult_mod(x, y, m);
            assert(z == U128{0});
        }
        {
            U128 x {3, 1};
            U128 y {1, 1};
            U128 m {1, 1};
            U128 z = mult_mod(x, y, m);
            assert(z == U128{0});
        }
        {
            U128 x {11, 5};
            U128 y {3, 7};
            U128 m {17, 13};
            U128 z = mult_mod(x, y, m);
            assert(z.value() == "151830893529763232515");
        }
    }

    void modular_inverse_test()
    {
        using namespace u128_utils;
        {
            U128 x {2};
            U128 m {13};
            bool ok;
            U128 y = modular_inverse(x, m, ok);
            assert(ok);
            assert(y.value() == "7");
        }
        {
            U128 x {2};
            U128 m {4};
            bool ok;
            U128 y = modular_inverse(x, m, ok);
            assert(!ok);
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
            assert(((q*y) / p).second == (x / p).second);
        }
        {
            const U128 x{35ull};
            const U128 y{7ull};
            const U128 p{13ull};
            auto q = div_mod(x, y, p);
            assert(((q*y) / p).second == (x / p).second);
        }
        {
            const U128 x{14ull};
            const U128 y{8ull};
            const U128 p{7ull};
            auto q = div_mod(x, y, p);
            assert(((q*y) / p).second == (x / p).second);
        }
        {
            const U128 x{0ull};
            const U128 y{8ull};
            const U128 p{8ull};
            auto q = div_mod(x, y, p);
            assert(((q*y) / p).second == (x / p).second);
        }
        {
            const U128 x{16ull};
            const U128 y{8ull};
            const U128 p{8ull};
            auto q = div_mod(x, y, p);
            assert(((q*y) / p).second == (x / p).second);
        }
    }

    void random_half_division_test(uint64_t min_value, uint64_t max_value, int num_of_parts, size_t number_of_iterations_per_part)
    {
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
// std::cout << "\tlast x // y = " << x.value() << " // " << y() << " = " << q.value() << ", remainder = " << r.value();
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
// std::cout << "\tlast x // y = " << x.value() << " // " << y.value() << " = " << q.value() << ", remainder = " << r.value();
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