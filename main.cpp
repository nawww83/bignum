#include <iostream>
#include "tests/u128_test.hpp"
#include "tests/i128_test.hpp"

int main()
{
    {
        using namespace tests_u128;

        std::cout << "Run all tests for U128...\n";

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

        {
            const auto num_of_runned_tests = division_test();
            std::cout << "Division tests: total number of tests run: " << num_of_runned_tests << std::endl;
        }

        {
            // Наибольшее значение половинок тестируемых чисел.
            // const uint64_t max_high_low_value = 128;
            // random_infinite_test(max_high_low_value);
        }
    }

    {
        using namespace tests_i128;

        std::cout << "Run all tests for I128...\n";

        debug_test();

        cmp_operator_test();
    }

    std::cout << "All Ok!\n";

    return 0;
}