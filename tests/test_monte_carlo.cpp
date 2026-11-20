#include <gtest/gtest.h>

#include <cmath>

#include "pricer/black_scholes.hpp"
#include "pricer/monte_carlo.hpp"

using namespace pricer;

namespace {
const Market kMkt{100.0, 0.05, 0.20, 0.01};
}

// Monte Carlo must converge to the closed form: within 4 std errors.
TEST(MonteCarlo, EuropeanConvergesToBlackScholes) {
    for (OptionType type : {OptionType::Call, OptionType::Put}) {
        const EuropeanOption opt{type, 105.0, 1.0};
        const auto res = mc::european(opt, kMkt, {200'000, 7, true});
        EXPECT_NEAR(res.price, bs::price(opt, kMkt), 4 * res.std_error);
    }
}

TEST(MonteCarlo, AntitheticReducesStdError) {
    const EuropeanOption opt{OptionType::Call, 100.0, 1.0};
    const auto plain = mc::european(opt, kMkt, {100'000, 1, false});
    const auto anti = mc::european(opt, kMkt, {100'000, 1, true});
    EXPECT_LT(anti.std_error, plain.std_error);
}

TEST(MonteCarlo, SameSeedSameResult) {
    const EuropeanOption opt{OptionType::Put, 100.0, 1.0};
    EXPECT_DOUBLE_EQ(mc::european(opt, kMkt, {10'000, 3, true}).price,
                     mc::european(opt, kMkt, {10'000, 3, true}).price);
}

// Averaging lowers volatility, so an Asian call is cheaper than the vanilla call.
TEST(MonteCarlo, AsianCallCheaperThanEuropean) {
    const EuropeanOption opt{OptionType::Call, 100.0, 1.0};
    const auto asian = mc::asian_arithmetic(opt, kMkt, 12, {100'000, 5, true});
    EXPECT_GT(asian.price, 0.0);
    EXPECT_LT(asian.price, bs::price(opt, kMkt));
}

// With one monitoring date, the Asian option *is* the European option.
TEST(MonteCarlo, AsianWithOneStepIsEuropean) {
    const EuropeanOption opt{OptionType::Call, 100.0, 1.0};
    const auto asian = mc::asian_arithmetic(opt, kMkt, 1, {200'000, 9, true});
    EXPECT_NEAR(asian.price, bs::price(opt, kMkt), 4 * asian.std_error);
}
