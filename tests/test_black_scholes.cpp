#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "pricer/black_scholes.hpp"

using namespace pricer;

namespace {
const Market kMkt{100.0, 0.05, 0.20, 0.0};
const EuropeanOption kCall{OptionType::Call, 100.0, 1.0};
const EuropeanOption kPut{OptionType::Put, 100.0, 1.0};
}  // namespace

// Reference values from Hull, "Options, Futures and Other Derivatives".
TEST(BlackScholes, MatchesReferenceValues) {
    EXPECT_NEAR(bs::price(kCall, kMkt), 10.4506, 1e-4);
    EXPECT_NEAR(bs::price(kPut, kMkt), 5.5735, 1e-4);
}

// C - P = S e^{-qT} - K e^{-rT} must hold for any parameters.
TEST(BlackScholes, PutCallParityWithDividends) {
    const Market mkt{95.0, 0.03, 0.35, 0.02};
    for (double K : {70.0, 95.0, 130.0}) {
        for (double T : {0.1, 1.0, 5.0}) {
            const double c = bs::price({OptionType::Call, K, T}, mkt);
            const double p = bs::price({OptionType::Put, K, T}, mkt);
            const double parity = mkt.spot * std::exp(-mkt.div_yield * T) -
                                  K * std::exp(-mkt.rate * T);
            EXPECT_NEAR(c - p, parity, 1e-10) << "K=" << K << " T=" << T;
        }
    }
}

// Every analytic greek must agree with a central finite difference of the price.
class GreeksFiniteDiff : public ::testing::TestWithParam<OptionType> {};

TEST_P(GreeksFiniteDiff, AnalyticMatchesNumeric) {
    const EuropeanOption opt{GetParam(), 110.0, 0.75};
    const Market mkt{100.0, 0.04, 0.25, 0.01};
    const Greeks g = bs::greeks(opt, mkt);
    const double h = 1e-4;

    auto bumped = [&](auto modify) {
        Market m = mkt;
        EuropeanOption o = opt;
        modify(m, o);
        return bs::price(o, m);
    };

    const double up_s = bumped([&](Market& m, EuropeanOption&) { m.spot += h; });
    const double dn_s = bumped([&](Market& m, EuropeanOption&) { m.spot -= h; });
    const double mid = bs::price(opt, mkt);
    EXPECT_NEAR(g.delta, (up_s - dn_s) / (2 * h), 1e-6);
    EXPECT_NEAR(g.gamma, (up_s - 2 * mid + dn_s) / (h * h), 1e-4);

    EXPECT_NEAR(g.vega,
                (bumped([&](Market& m, EuropeanOption&) { m.vol += h; }) -
                 bumped([&](Market& m, EuropeanOption&) { m.vol -= h; })) / (2 * h),
                1e-5);
    EXPECT_NEAR(g.rho,
                (bumped([&](Market& m, EuropeanOption&) { m.rate += h; }) -
                 bumped([&](Market& m, EuropeanOption&) { m.rate -= h; })) / (2 * h),
                1e-5);
    // Theta is the derivative with respect to calendar time, i.e. -dV/dT.
    EXPECT_NEAR(g.theta,
                -(bumped([&](Market&, EuropeanOption& o) { o.maturity += h; }) -
                  bumped([&](Market&, EuropeanOption& o) { o.maturity -= h; })) / (2 * h),
                1e-5);
}

INSTANTIATE_TEST_SUITE_P(CallAndPut, GreeksFiniteDiff,
                         ::testing::Values(OptionType::Call, OptionType::Put));

TEST(BlackScholes, RejectsInvalidInputs) {
    EXPECT_THROW(bs::price(kCall, {0.0, 0.05, 0.2, 0.0}), std::invalid_argument);
    EXPECT_THROW(bs::price(kCall, {100.0, 0.05, -0.2, 0.0}), std::invalid_argument);
    EXPECT_THROW(bs::price({OptionType::Call, 100.0, 0.0}, kMkt), std::invalid_argument);
    EXPECT_THROW(bs::price({OptionType::Call, std::nan(""), 1.0}, kMkt), std::invalid_argument);
}
