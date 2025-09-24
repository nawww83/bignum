#include <iostream>
#include "tests/u128_test.hpp"
#include "tests/i128_test.hpp"

int main()
{
    constexpr bool RUN_LONG_TESTS = true;
    {
        using namespace tests_u128;

        std::cout << "Run quick U128 tests...";

        debug_test();

        string_value_test();

        cmp_operator_test();

        shift_right_operator_test();

        shift_left_operator_test();

        and_operator_test();

        or_operator_test();

        xor_operator_test();

        addition_test();

        subtraction_test();

        multiplication_test();

        const auto num_of_runned_tests = division_test();

        reciprocal_test();

        bit_length_test();

        std::cout << "Ok!\n";
    }

    {
        using namespace tests_i128;

        std::cout << "Run quick I128 tests...";

        debug_test();

        cmp_operator_test();

        addition_test();

        subtraction_test();

        multiplication_test();

        division_test();

        isqrt_test();

        std::cout << "Ok!\n";
    }

    if (RUN_LONG_TESTS)
    { // Долгие тесты.
        using namespace tests_u128;
        constexpr size_t ITERS_PER_PART = 1024ull * 65536ull;
        // Количество частей. После каждой части происходит вывод на экран.
        constexpr int NUM_OF_PARTS = 8;
        {
            // Наименьшее значение половинок тестируемых чисел.
            const uint64_t min_high_low_value = -32ull;
            // Наибольшее значение половинок тестируемых чисел.
            const uint64_t max_high_low_value = 32ull;
            random_half_division_test(min_high_low_value, max_high_low_value, NUM_OF_PARTS, ITERS_PER_PART);
        }
        {
            // Наименьшее значение половинок тестируемых чисел.
            const uint64_t min_high_low_value = -32ull;
            // Наибольшее значение половинок тестируемых чисел.
            const uint64_t max_high_low_value = 32ull;
            random_full_division_test(min_high_low_value, max_high_low_value, NUM_OF_PARTS, ITERS_PER_PART);
        }
        // Неограниченные по диапазону числа.
        random_half_division_test(1, 0, NUM_OF_PARTS, ITERS_PER_PART);
        random_full_division_test(1, 0, NUM_OF_PARTS, ITERS_PER_PART);
    }

    std::cout << "All Ok! Exit... don't forget flush...\n";
    std::cout << std::flush;

    return 0;
}