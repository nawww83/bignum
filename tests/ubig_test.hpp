/**
 * @author nawww83@gmail.com
 * @brief Тесты арифметики N-битных беззнаковых целых чисел, N = 256, 512, ...
 */

#pragma once

namespace tests_ubig
{
    void debug_test();

    void test_u256_division();

    void stress_test_u256(int iterations = 100000);
}