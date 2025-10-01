#include "ubig_test.hpp"
#include "../ubig.hpp"
#include "../u128.hpp"
#include <cassert>

using namespace bignum::ubig;
using U128 = bignum::u128::U128;
using U256 = UBig<U128, 256>;

namespace tests_ubig
{
    void debug_test() 
    {
        {
            U256 x{1};
            U256 y{1};
            auto z = x + y;
            assert(z.value() == "2");
        } 
        {
            const U256 x = U256::get_max_value();
            assert(x.value() == "115792089237316195423570985008687907853269984665640564039457584007913129639935");
            assert(x.bit_length() == 256);
        }     
    }
}