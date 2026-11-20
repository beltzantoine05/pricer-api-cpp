#include <gtest/gtest.h>

#include <stdexcept>

#include "pricer/black_scholes.hpp"
#include "pricer/implied_vol.hpp"

using namespace pricer;

// Price with a known vol, then recover that vol from the price.
TEST(ImpliedVol, RoundTrip) {
    for (OptionType type : {OptionType::Call, OptionType::Put}) {
        for (double K : {60.0, 90.0, 100.0, 115.0, 160.0}) {
            for (double vol : {0.05, 0.2, 0.6, 1.5}) {
                const EuropeanOption opt{type, K, 0.5};
                const Market mkt{100.0, 0.03, vol, 0.01};
                const double px = bs::price(opt, mkt);
                // Near-zero vega (deep ITM or OTM at low vol): the price carries
                // no time value, so the vol cannot be recovered from it.
                if (bs::greeks(opt, mkt).vega < 1e-4) continue;
                EXPECT_NEAR(implied_vol(px, opt, mkt), vol, 1e-6)
                    << "K=" << K << " vol=" << vol;
            }
        }
    }
}

TEST(ImpliedVol, RejectsArbitragePrices) {
    const EuropeanOption call{OptionType::Call, 100.0, 1.0};
    const Market mkt{100.0, 0.0, 0.2, 0.0};
    EXPECT_THROW(implied_vol(-1.0, call, mkt), std::invalid_argument);   // negative
    EXPECT_THROW(implied_vol(150.0, call, mkt), std::invalid_argument);  // above spot
}
