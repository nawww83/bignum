#include "ubig_test.hpp"
#include "../ubig.hpp"
#include "../u128.hpp"
#include <cassert>
#include <iostream>

using namespace bignum;
using U128 = bignum::u128::U128;
using U256 = UBig<U128>;
using U512 = UBig<U256>;

namespace tests_ubig
{
    void debug_test() 
    {
        {
            U256 x{1};
            U256 y{1};
            auto z = x + y;
            assert(z.toString() == "2");
        } 
        {
            const U256 x = U256::max();
            assert(x.toString() == "115792089237316195423570985008687907853269984665640564039457584007913129639935");
            assert(x.bit_width() == 256);
        }
        {
            U128 x = U128::max();
            auto y = U256::square_ext(x);
            assert(y.toString() == "115792089237316195423570985008687907852589419931798687112530834793049593217025");
        } 
        //
        {
            U512 x{1};
            U512 y{1};
            auto z = x + y;
            assert(z.toString() == "2");
        } 
        {
            const U512 x = U512::max();
            assert(x.toString() == "13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006084095");
            assert(x.bit_width() == 512);
        } 
    }
}