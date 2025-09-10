#include <iostream>
#include "tests/u128_test.hpp"
#include "tests/i128_test.hpp"

int main()
{
    constexpr bool RUN_LONG_TESTS = true;
    {
        using namespace tests_u128;

        std::cout << "Run all tests for U128...\n";

        // debug_test();

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

        {
            const auto num_of_runned_tests = division_test();
            std::cout << "Division tests: total number of tests run: " << num_of_runned_tests << std::endl;
        }
    }

    {
        using namespace tests_i128;

        std::cout << "Run all tests for I128...\n";

        debug_test();

        cmp_operator_test();

        addition_test();

        subtraction_test();

        mulpiplication_test();
    }

    if (RUN_LONG_TESTS) { // Долгие тесты.
        using namespace tests_u128;
        // Количество выводов счетчиков на экран.
        constexpr int num_of_debug_prints = 3;
        {
            // Наименьшее значение половинок тестируемых чисел.
            const uint64_t min_high_low_value = -128ull;
            // Наибольшее значение половинок тестируемых чисел.
            const uint64_t max_high_low_value = 128ull;
            random_half_division_test(min_high_low_value, max_high_low_value, num_of_debug_prints);
        }
        {
            // Наименьшее значение половинок тестируемых чисел.
            const uint64_t min_high_low_value = -128ull;
            // Наибольшее значение половинок тестируемых чисел.
            const uint64_t max_high_low_value = 128ull;
            random_full_division_test(min_high_low_value, max_high_low_value, num_of_debug_prints);
        }
        // Неограниченные по диапазону числа.
        {
            random_half_division_test(1, 0, num_of_debug_prints);
        }
        {
            random_full_division_test(1, 0, num_of_debug_prints);
        }
    }

    std::cout << "All Ok! Exit... (don't forget flush...)\n";
    std::cout << std::flush;

    return 0;
}