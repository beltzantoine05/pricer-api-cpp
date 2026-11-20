#include "pricer/implied_vol.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "pricer/black_scholes.hpp"

namespace pricer {

double implied_vol(double market_price, const EuropeanOption& opt, const Market& mkt,
                   const ImpliedVolConfig& cfg) {
    // No-arbitrage bounds: an option is worth at least its discounted intrinsic
    // value, and a call is worth less than the (discounted) spot.
    const double df_r = std::exp(-mkt.rate * opt.maturity);
    const double fwd_spot = mkt.spot * std::exp(-mkt.div_yield * opt.maturity);
    const double k_disc = opt.strike * df_r;
    const bool is_call = opt.type == OptionType::Call;
    const double lower = is_call ? std::max(fwd_spot - k_disc, 0.0)
                                 : std::max(k_disc - fwd_spot, 0.0);
    const double upper = is_call ? fwd_spot : k_disc;
    if (!(market_price > lower && market_price < upper))
        throw std::invalid_argument("market price outside no-arbitrage bounds");

    Market m = mkt;  // a copy we can modify
    double lo = cfg.vol_low, hi = cfg.vol_high;
    double sigma = 0.2;  // a sensible first guess for most markets

    for (int i = 0; i < cfg.max_iterations; ++i) {
        m.vol = sigma;
        const double diff = bs::price(opt, m) - market_price;
        if (std::abs(diff) < cfg.tolerance) return sigma;

        // Price is increasing in vol, so diff tells us which side the root is on.
        if (diff > 0.0) hi = sigma; else lo = sigma;

        const double vega = bs::greeks(opt, m).vega;
        double next = sigma - diff / vega;              // Newton step
        if (!(next > lo && next < hi)) next = 0.5 * (lo + hi);  // bisection fallback
        sigma = next;
    }
    throw std::runtime_error("implied vol did not converge");
}

}  // namespace pricer
