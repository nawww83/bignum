#include <cassert>
#include <iostream>
#include "../i128.hpp"

using namespace bignum::i128;

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
            const auto &x_str = x.value();
            assert(x_str == "1");
        }
        {
            auto x = -I128{1};
            const auto &x_str = x.value();
            assert(x_str == "-1");
        }
        {
            auto x = -I128{0};
            const auto &x_str = x.value();
            assert(x_str == "0");
        }
        {
            auto x = -I128{0, 1};
            const auto &x_str = x.value();
            assert(x_str == "-18446744073709551616");
        }
        {
            auto x = I128{U128{1}, Sign{false}, Singular{true}};
            const auto &x_str = x.value();
            assert(x_str == "inf");
        }
        {
            auto x = I128{U128{1}, Sign{false}, Singular{false, true}};
            const auto &x_str = x.value();
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
            I128 x{U128{1, 1}};
            I128 y{U128{1, 1}};
            I128 z = x * y;
            assert(z.is_overflow());
        }
        {
            I128 x{U128{0, 1}};
            ULOW y{-1ull};
            I128 z = x * y;
            assert(!z.is_overflow());
            assert((z == I128{U128{0, 18446744073709551615ull}}));
        }
        {
            I128 x{U128{0, 1}};
            ULOW y{-1ull};
            I128 z = y * x;
            assert(!z.is_overflow());
            assert((z == I128{U128{0, 18446744073709551615ull}}));
        }
        {
            I128 x{U128{1, 1}};
            ULOW y{-1ull};
            I128 z = x * y;
            assert(!z.is_overflow());
            assert((z == I128{U128{18446744073709551615ull, 18446744073709551615ull}}));
        }
        {
            I128 x{U128{0, 2}};
            ULOW y{-1ull};
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
            ULOW y{1};
            const auto &[q, r] = x / y;
            assert(q == I128{555ull});
            assert(r == 0);
        }
        {
            I128 x{U128{555}, Sign{true}};
            ULOW y{1};
            const auto &[q, r] = x / y;
            assert(q == -I128{555ull});
            assert(r == 0);
        }
        {
            I128 x{U128{555}};
            ULOW y{7};
            const auto &[q, r] = x / y;
            assert(q == I128{79ull});
            assert(r == 2ull);
        }
        {
            I128 x{U128{555}, Sign{true}};
            ULOW y{7};
            const auto &[q, r] = x / y;
            assert(q == -I128{80ull});
            assert(r == 5ull);
        }
        {
            I128 x{U128{444}};
            ULOW y{2};
            const auto &[q, r] = x / y;
            assert(q == I128{222ull});
            assert(r == 0);
        }
        {
            I128 x{U128{444}, Sign{true}};
            ULOW y{2};
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
            assert(q == -I128{80ull});
            assert(r == I128{5ull});
        }
        {
            I128 x{U128{555}};
            I128 y{U128{7}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == -I128{80ull});
            assert(r == -I128{5ull});
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
            assert(q == -I128{1ull});
            assert(r == I128{2ull});
        }
        {
            I128 x{U128{5}};
            I128 y{U128{7}, Sign{true}};
            const auto &[q, r] = x / y;
            assert(q == -I128{1ull});
            assert(r == -I128{2ull});
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
}