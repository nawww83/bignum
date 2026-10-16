#include <cassert>
#include <iostream>
#include "../i128.hpp"
#include "../i128_utils.hpp"

using namespace bignum::i128;
using u64 = uint64_t;

namespace tests_i128
{
    void debug_test()
    {
        ;
    }

    void string_value_test()
    {
        {
            I128 x{1};
            const auto &x_str = x.toString();
            assert(x_str == "1");
        }
        {
            auto x = -I128{1};
            const auto &x_str = x.toString();
            assert(x_str == "-1");
        }
        {
            auto x = -I128{0};
            const auto &x_str = x.toString();
            assert(x_str == "0");
        }
        {
            auto x = -I128{0, 1};
            const auto &x_str = x.toString();
            assert(x_str == "-18446744073709551616");
        }
        {
            auto x = I128{U128{1}, Sign{false}, Singular{true}};
            const auto &x_str = x.toString();
            assert(x_str == "inf");
        }
        {
            auto x = I128{U128{1}, Sign{false}, Singular{false, true}};
            const auto &x_str = x.toString();
            assert(x_str == "nan");
        }
    }

    void cmp_operator_test()
    {
        {
            I128 x{U128{1, 1}};
            I128 y{U128{1, 1}};
            assert(x == y);
        }
        {
            I128 x{U128{1, 1}, Sign{true}};
            I128 y{U128{1, 1}};
            assert(x != y);
            assert(x < y);
        }
        {
            I128 x{U128{1, 1}};
            I128 y{U128{1, 1}, Sign{true}};
            assert(x != y);
            assert(x > y);
        }
        {
            I128 x{U128{1, 1}, Sign{true}};
            I128 y{U128{1, 1}, Sign{true}};
            assert(x == y);
        }
        {
            I128 x{U128{0, 1}};
            I128 y{U128{1, 1}, Sign{true}};
            assert(x > y);
        }
        {
            I128 x{U128{0, 1}};
            I128 y{U128{1, 1}, Sign{true}};
            assert(y < x);
        }
        {
            I128 x{U128{0, 1}, Sign{true}};
            I128 y{U128{1, 1}};
            assert(x < y);
        }
        {
            I128 x{U128{0, 1}, Sign{true}};
            I128 y{U128{1, 1}};
            assert(y > x);
        }
        {
            I128 x{U128{0}, Sign{true}};
            I128 y{U128{0}};
            assert(x == y);
        }
        {
            I128 x{U128{0}};
            I128 y{U128{0}, Sign{true}};
            assert(x == y);
        }
        {
            I128 x{U128{0}, Sign{true}};
            I128 y{U128{0}, Sign{true}};
            assert(x == y);
        }
        {
            I128 x{U128{0}, Sign{true}, Singular{true}};
            I128 y{U128{0}};
            assert(x != y);
        }
        {
            I128 x{U128{0}, Sign{false}, Singular{true}};
            I128 y{U128{0}, Sign{true}};
            assert(x != y);
        }
        {
            I128 x{U128{0}, Sign{false}, Singular{true}};
            I128 y{U128{0}};
            assert(x != y);
        }
        {
            I128 x{U128{0}, Sign{false}, Singular{false, true}};
            I128 y{U128{0}};
            assert(x != y);
        }
        {
            I128 x{U128{0}, Sign{false}, Singular{true, false}};
            I128 y{U128{0}};
            assert(x != y);
        }
        {
            I128 x{U128{0}, Sign{false}, Singular{true, true}};
            I128 y{U128{0}};
            assert(x != y);
        }
        // Регрессия: сравнение с нулём при разных знаках.
        {
            I128 zero{U128{0}};
            I128 neg{U128{5}, Sign{true}}; // -5
            assert(!(zero == neg));
            assert(zero != neg);
            assert(zero > neg);
            assert(zero >= neg);
            assert(!(zero < neg));
            assert(!(zero <= neg));
        }
        {
            I128 neg_zero{U128{0}, Sign{true}}; // -0
            I128 pos{U128{5}};
            assert(neg_zero < pos);
            assert(neg_zero <= pos);
            assert(!(neg_zero > pos));
            assert(!(neg_zero >= pos));
        }
        {
            // ±0 равны между собой.
            I128 neg_zero{U128{0}, Sign{true}};
            I128 pos_zero{U128{0}};
            assert(neg_zero == pos_zero);
        }
    }

    void shift_operator_test()
    {
        // Регрессия: operator>> раньше сдвигал влево.
        {
            I128 x{U128{16}};
            assert((x >> 1) == I128{U128{8}});
            assert((x >> 2) == I128{U128{4}});
        }
        {
            I128 x{U128{1, 1}}; // 2^64 + 1
            I128 y = x >> 1;    // 2^63
            assert(y == I128{U128{1ull << 63}});
        }
        {
            I128 x{U128{16}};
            x >>= 3;
            assert(x == I128{U128{2}});
        }
        {
            // Сдвиг влево продолжает работать.
            I128 x{U128{1}};
            assert((x << 4) == I128{U128{16}});
        }
        {
            // Знак при сдвиге сохраняется.
            I128 x{U128{16}, Sign{true}};
            const I128 expected{U128{4}, Sign{true}};
            assert((x >> 2) == expected);
        }
        {
            // Большой сдвиг даёт ноль.
            I128 x{U128{7}};
            assert((x >> 200) == I128{U128{0}});
        }
    }

    void addition_test()
    {
        {
            I128 x{U128{1}};
            I128 y{U128{1}};
            I128 z = x + y;
            assert(z == I128{U128{2}});
        }
        {
            I128 x{U128{3, 1}};
            I128 y{U128{1, 2}};
            I128 z = x + y;
            assert((z == I128{U128{4, 3}}));
        }
        {
            I128 x{U128{-1ull}};
            I128 y{U128{1}};
            I128 z = x + y;
            assert((z == I128{U128{0, 1}}));
        }
        {
            I128 x{U128{-2ull, -1ull}};
            I128 y{U128{1}};
            I128 z = x + y;
            assert(!z.is_overflow());
        }
        {
            I128 x{U128{-1ull, -1ull}};
            I128 y{U128{1}};
            I128 z = x + y;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{1}};
            I128 y{U128{1}}; y.set_overflow();
            I128 z = x + y;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{1}};
            I128 y{U128{1}}; y.set_nan();
            I128 z = x + y;
            assert(z.is_nan());
        }
    }

    void subtraction_test()
    {
        {
            I128 x{U128{1}};
            I128 y{U128{1}};
            I128 z = x - y;
            assert(z.is_zero());
        }
        {
            I128 x{U128{1}};
            I128 y{U128{2}};
            I128 z = x - y;
            assert((z == I128{U128{1}, Sign{true}}));
        }
        {
            I128 x{U128{0}};
            I128 y{U128{1}};
            I128 z = x - y;
            assert((z == I128{U128{1}, Sign{true}}));
        }
        {
            I128 x{U128{8}};
            I128 y{U128{3}};
            I128 z = x - y;
            assert(z == I128{U128{5}});
        }
        {
            I128 x{U128{1, 2}};
            I128 y{U128{2, 1}};
            I128 z = x - y;
            assert((z == I128{U128{-1ull, 0}}));
        }
        {
            I128 x{U128{2, 1}};
            I128 y{U128{1, 2}};
            I128 z = x - y;
            assert((z == I128{U128{-1ull, 0}, Sign{true}}));
        }
        {
            I128 x{U128{-1ull, -1ull}, Sign{true}};
            I128 y{U128{1}};
            I128 z = x - y;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{1}};
            I128 y{U128{1}}; y.set_overflow();
            I128 z = x - y;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{1}};
            I128 y{U128{1}}; y.set_nan();
            I128 z = x - y;
            assert(z.is_nan());
        }
    }

    void multiplication_test()
    {
        {
            I128 x{U128{8}};
            I128 y{U128{3}};
            I128 z = x * y;
            assert(z == I128{U128{24}});
        }
        {
            I128 x{U128{8}, Sign{true}};
            I128 y{U128{3}};
            I128 z = x * y;
            assert(z == -I128{U128{24}});
        }
        {
            I128 x{U128{8}};
            I128 y{U128{3}, Sign{true}};
            I128 z = x * y;
            assert((z == I128{U128{24}, Sign{true}}));
        }
        {
            I128 x{U128{8}, Sign{true}};
            I128 y{U128{3}, Sign{true}};
            I128 z = x * y;
            assert(z == I128{U128{24}});
        }
        {
            I128 x{U128{123}, Sign{true}};
            I128 y{U128{2, 2}};
            I128 z = x * y;
            const auto& w = U128{246ull, 246ull};
            assert((z == I128{w, Sign{true}}));
        }
        {
            I128 x{U128{1, 1}};
            I128 y{U128{1, 1}};
            I128 z = x * y;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{0, 1}};
            u64 y{-1ull};
            I128 z = x * y;
            assert(!z.is_overflow());
            assert((z == I128{U128{0, 18446744073709551615ull}}));
        }
        {
            I128 x{U128{0, 1}};
            u64 y{-1ull};
            I128 z = x * y;
            assert(!z.is_overflow());
            assert((z == I128{U128{0, 18446744073709551615ull}}));
        }
        {
            I128 x{U128{1, 1}};
            u64 y{-1ull};
            I128 z = x * y;
            assert(!z.is_overflow());
            assert((z == I128{U128{18446744073709551615ull, 18446744073709551615ull}}));
        }
        {
            I128 x{U128{0, 2}};
            u64 y{-1ull};
            I128 z = x * y;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{0}};
            I128 y{U128{1}}; y.set_overflow();
            I128 z = x * y;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{0}};
            I128 y{U128{1}}; y.set_nan();
            I128 z = x * y;
            assert(z.is_nan());
        }
    }

    void division_test()
    {
        {
            I128 x{U128{555}};
            u64 y{1};
            const auto &[q, r] = x / y;
            assert(q == I128{555ull});
            assert(r == 0);
        }
        {
            I128 x{U128{555}, Sign{true}};
            u64 y{1};
            const auto &[q, r] = x / y;
            assert(q == -I128{555ull});
            assert(r == 0);
        }
        {
            I128 x{U128{555}};
            u64 y{7};
            const auto &[q, r] = x / y;
            assert(q == I128{79ull});
            assert(r == 2ull);
        }
        {
            I128 x{U128{555}, Sign{true}};
            u64 y{7};
            const auto &[q, r] = x / y;
            // Деление усекается к нулю: -555 = 7*(-79) + (-2), остаток берётся по модулю.
            assert(q == -I128{79ull});
            assert(r == 2ull);
        }
        {
            I128 x{U128{444}};
            u64 y{2};
            const auto &[q, r] = x / y;
            assert(q == I128{222ull});
            assert(r == 0);
        }
        {
            I128 x{U128{444}, Sign{true}};
            u64 y{2};
            const auto &[q, r] = x / y;
            assert(q == -I128{222ull});
            assert(r == 0);
        }
        //
        {
            I128 x{U128{555}, Sign{true}};
            I128 y{U128{1}};
            const auto &[q, r] = x / y;
            assert(q == -I128{555ull});
            assert(r == I128{0});
        }
        {
            I128 x{U128{555}};
            I128 y{U128{1}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == -I128{555ull});
            assert(r == I128{0});
        }
        {
            I128 x{U128{555}, Sign{true}};
            I128 y{U128{1}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == I128{555ull});
            assert(r == I128{0});
        }
        {
            I128 x{U128{555}};
            I128 y{U128{7}};
            const auto &[q, r] = x / y;
            assert(q == I128{79ull});
            assert(r == I128{2ull});
        }
        {
            I128 x{U128{555}, Sign{true}};
            I128 y{U128{7}};
            const auto &[q, r] = x / y;
            // Усечение к нулю: модуль частного |−555|/7 = 79, знак в sign.
            assert(q == -I128{79ull});
            assert(r == -I128{2ull});
        }
        {
            I128 x{U128{555}};
            I128 y{U128{7}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == -I128{79ull});
            assert(r == I128{2ull});
        }
        {
            I128 x{U128{555}, Sign{true}};
            I128 y{U128{7}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == I128{79ull});
            assert(r == -I128{2ull});
        }
        {
            I128 x{U128{5}};
            I128 y{U128{7}};
            const auto &[q, r] = x / y;
            assert(q == I128{0});
            assert(r == I128{5ull});
        }
        {
            I128 x{U128{5}, Sign{true}};
            I128 y{U128{7}};
            const auto &[q, r] = x / y;
            // Усечение к нулю: -5 / 7 = 0 (со знаком), остаток -5.
            assert(q == I128{0});
            assert(r == -I128{5ull});
        }
        {
            I128 x{U128{5}};
            I128 y{U128{7}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == I128{0});
            assert(r == I128{5ull});
        }
        {
            I128 x{U128{5}, Sign{true}};
            I128 y{U128{7}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == I128{0});
            assert(r == -I128{5ull});
        }
        {
            I128 x{U128{444}};
            I128 y{U128{2}};
            const auto &[q, r] = x / y;
            assert(q == I128{222ull});
            assert(r == I128{0});
        }
        {
            I128 x{U128{444}, Sign{true}};
            I128 y{U128{2}};
            const auto &[q, r] = x / y;
            assert(q == -I128{222ull});
            assert(r == I128{0});
        }
        {
            I128 x{U128{444}};
            I128 y{U128{2}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == -I128{222ull});
            assert(r == I128{0});
        }
        {
            I128 x{U128{444}, Sign{true}};
            I128 y{U128{2}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == I128{222ull});
            assert(r == I128{0});
        }
        {
            I128 x{U128{0}};
            I128 y{U128{1}}; y.set_overflow();
            I128 z = (x / y).first;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{0}};
            I128 y{U128{1}}; y.set_nan();
            I128 z = (x / y).first;
            assert(z.is_nan());
        }
    }

    void division_sign_test()
    {
        // Регрессия: знак частного хранится только в поле sign, модуль равен
        // |X|/|Y| (деление усекается к нулю, без евклидовой коррекции Q -= 1).
        // Из-за прежней коррекции, например, I128{-3} / 2 давало -2 вместо -1,
        // что ломало вычисления Decimal (в частности, вычитание дробей).
        {
            const auto &[q, r] = I128{U128{3}, Sign{true}} / I128{U128{2}};   // -3 / 2
            assert(q == -I128{1ull});
            assert(r == -I128{1ull});
        }
        {
            const auto &[q, r] = I128{U128{7}, Sign{true}} / I128{U128{2}};   // -7 / 2
            assert(q == -I128{3ull});
            assert(r == -I128{1ull});
        }
        {
            const auto &[q, r] = I128{U128{7}} / I128{U128{2}, Sign{true}};   // 7 / -2
            assert(q == -I128{3ull});
            assert(r == I128{1ull});
        }
        {
            const auto &[q, r] = I128{U128{7}, Sign{true}} / I128{U128{2}, Sign{true}}; // -7 / -2
            assert(q == I128{3ull});
            assert(r == -I128{1ull});
        }
        {
            // Модуль частного не зависит от знаков операндов.
            const I128 d{U128{7}};
            const I128 q_pos = (I128{U128{100}} / d).first;
            const I128 q_neg = (I128{U128{100}, Sign{true}} / d).first;
            assert(q_pos == I128{14ull});
            assert(q_neg == -I128{14ull});
            assert(q_pos.unsigned_part() == q_neg.unsigned_part());
        }
        {
            // Полуширинное деление: знак также только в sign, остаток — по модулю.
            const auto &[q, r] = I128{U128{555}, Sign{true}} / u64{7}; // -555 / 7
            assert(q == -I128{79ull});
            assert(r == 2ull);
        }
    }

    void isqrt_test()
    {
        using namespace i128_utils;
        {
            I128 x{U128{4}};
            bool exact;
            auto y = isqrt(x, exact);
            assert(y == I128{2});
            assert(exact);
        }
        {
            I128 x{U128{5}};
            bool exact;
            auto y = isqrt(x, exact);
            assert(y == I128{2});
            assert(!exact);
        }
        {
            I128 x{U128{0}};
            bool exact;
            auto y = isqrt(x, exact);
            assert(y == I128{0});
            assert(exact);
        }
        {
            I128 x{U128{1}};
            bool exact;
            auto y = isqrt(x, exact);
            assert(y == I128{1});
            assert(exact);
        }
        {
            I128 x{625ull};
            bool exact;
            auto y = isqrt(x, exact);
            assert(y == I128{25ull});
            assert(exact);
        }
        {
            I128 x{U128{1}};
            x.set_overflow();
            auto y = isqrt(x);
            assert(y.is_overflow());
        }
        {
            I128 x{U128{11817193982676505668ull, 8262476398185226452ull}};
            bool exact;
            auto y = isqrt(x, exact);
            assert(y == I128{12345678901234567890ull});
            assert(exact);
        }
    }
}