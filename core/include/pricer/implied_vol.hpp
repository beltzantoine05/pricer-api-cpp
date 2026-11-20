#pragma once
#include "pricer/types.hpp"

namespace pricer {

struct ImpliedVolConfig {
    double tolerance = 1e-10;   // stop when |model price - market price| < tolerance
    int max_iterations = 100;
    double vol_low = 1e-6;      // search interval for sigma
    double vol_high = 5.0;
};

// Finds sigma such that bs::price(opt, mkt with vol = sigma) == market_price.
// Newton-Raphson (fast, uses vega), falling back to bisection when a Newton
// step leaves the bracket. mkt.vol is ignored.
// Throws std::invalid_argument if market_price violates no-arbitrage bounds.
double implied_vol(double market_price, const EuropeanOption& opt, const Market& mkt,
                   const ImpliedVolConfig& cfg = {});

}  // namespace pricer
