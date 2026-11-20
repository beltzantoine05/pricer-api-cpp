#include "pricer/black_scholes.hpp"

#include <cmath>
#include <stdexcept>

namespace pricer {

void validate(const EuropeanOption& opt, const Market& mkt) {
    if (!(mkt.spot > 0.0)) throw std::invalid_argument("spot must be > 0");
    if (!(opt.strike > 0.0)) throw std::invalid_argument("strike must be > 0");
    if (!(opt.maturity > 0.0)) throw std::invalid_argument("maturity must be > 0");
    if (!(mkt.vol > 0.0)) throw std::invalid_argument("vol must be > 0");
    // Written as !(x > 0) rather than x <= 0 so that NaN inputs are rejected too.
}

namespace bs {

namespace {  // anonymous namespace: these helpers are private to this file

constexpr double kInvSqrt2 = 0.70710678118654752440;
constexpr double kInvSqrt2Pi = 0.39894228040143267794;

// Everything the formulas share, computed once.
struct Terms {
    double d1, d2, sqrt_t, df_r, df_q;  // df = discount factor
};

Terms terms(const EuropeanOption& opt, const Market& mkt) {
    const double sqrt_t = std::sqrt(opt.maturity);
    const double vol_sqrt_t = mkt.vol * sqrt_t;
    const double d1 = (std::log(mkt.spot / opt.strike) +
                       (mkt.rate - mkt.div_yield + 0.5 * mkt.vol * mkt.vol) * opt.maturity) /
                      vol_sqrt_t;
    return {d1, d1 - vol_sqrt_t, sqrt_t,
            std::exp(-mkt.rate * opt.maturity), std::exp(-mkt.div_yield * opt.maturity)};
}

}  // namespace

// erfc is more accurate than 1 + erf for very negative x (deep OTM options).
double norm_cdf(double x) { return 0.5 * std::erfc(-x * kInvSqrt2); }
double norm_pdf(double x) { return kInvSqrt2Pi * std::exp(-0.5 * x * x); }

double price(const EuropeanOption& opt, const Market& mkt) {
    validate(opt, mkt);
    const auto [d1, d2, sqrt_t, df_r, df_q] = terms(opt, mkt);  // structured binding (C++17 feature)
    const double S = mkt.spot, K = opt.strike;

    if (opt.type == OptionType::Call)
        return S * df_q * norm_cdf(d1) - K * df_r * norm_cdf(d2);
    return K * df_r * norm_cdf(-d2) - S * df_q * norm_cdf(-d1);
}

Greeks greeks(const EuropeanOption& opt, const Market& mkt) {
    validate(opt, mkt);
    const auto [d1, d2, sqrt_t, df_r, df_q] = terms(opt, mkt);
    const double S = mkt.spot, K = opt.strike, r = mkt.rate, q = mkt.div_yield;
    const double pdf_d1 = norm_pdf(d1);

    // Gamma and vega are the same for calls and puts.
    Greeks g{};
    g.gamma = df_q * pdf_d1 / (S * mkt.vol * sqrt_t);
    g.vega = S * df_q * pdf_d1 * sqrt_t;
    const double time_decay = -S * df_q * pdf_d1 * mkt.vol / (2.0 * sqrt_t);

    if (opt.type == OptionType::Call) {
        g.delta = df_q * norm_cdf(d1);
        g.theta = time_decay - r * K * df_r * norm_cdf(d2) + q * S * df_q * norm_cdf(d1);
        g.rho = K * opt.maturity * df_r * norm_cdf(d2);
    } else {
        g.delta = df_q * (norm_cdf(d1) - 1.0);
        g.theta = time_decay + r * K * df_r * norm_cdf(-d2) - q * S * df_q * norm_cdf(-d1);
        g.rho = -K * opt.maturity * df_r * norm_cdf(-d2);
    }
    return g;
}

}  // namespace bs
}  // namespace pricer
