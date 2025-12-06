#include <iostream>
#include "benchmark.hpp"
#include "../u128.hpp"
#include "../ubig.hpp"

using namespace bignum;

namespace bench {
    using U128 = u128::U128;
    using U256 = ubig::UBig<U128, 256>;

    U128 poly_mod(const U128& x, const U128& p) {
        U256 y = U256::square_ext(x);
        const auto& [q, r] = y / p;
        return r;
    }
    
    void modulo_poly_calc()
    {
        U128 x{11372209130871503813ull, 799616663795765462ull};
        U128 p{1857756895516871747ull};
        auto duration = benchmark<U128>(poly_mod, x, p);
        std::cout << "Duration: " << duration << '\n';
    }
}