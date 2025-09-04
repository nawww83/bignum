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
}