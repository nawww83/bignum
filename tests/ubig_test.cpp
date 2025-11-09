#include "ubig_test.hpp"
#include "../ubig.hpp"
#include "../u128.hpp"
#include <cassert>

using namespace bignum::ubig;
using U128 = bignum::u128::U128;
using U256 = UBig<U128, 256>;
using U512 = UBig<U256, 512>;

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
        {
            U128 x = U128::get_max_value();
            auto y = U256::square(x);
            assert(y.value() == "115792089237316195423570985008687907852589419931798687112530834793049593217025");
        } 
        //
        {
            U512 x{1};
            U512 y{1};
            auto z = x + y;
            assert(z.value() == "2");
        } 
        {
            const U512 x = U512::get_max_value();
            assert(x.value() == "13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006084095");
            assert(x.bit_length() == 512);
        } 
    }
}