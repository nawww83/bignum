/**
 * @author nawww83@gmail.com
 * @brief Тесты арифметики 128-битных беззнаковых целых чисел.
 */

#pragma once

namespace tests_u128
{
    void debug_test();

    void string_value_test();

    void cmp_operator_test();

    void shift_right_operator_test();

    void shift_left_operator_test();

    void and_operator_test();

    void or_operator_test();

    void xor_operator_test();

    void addition_test();

    void subtraction_test();

    void multiplication_test();

    int division_test();

    void random_infinite_test(uint64_t max_value);
}