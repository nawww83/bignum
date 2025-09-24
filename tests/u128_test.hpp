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

    void reciprocal_test();

    void bit_length_test();

    void random_half_division_test(uint64_t min_value, uint64_t max_value, int num_of_parts, size_t number_of_iterations_per_part);

    void random_full_division_test(uint64_t min_value, uint64_t max_value, int num_of_parts, size_t number_of_iterations_per_part);
}