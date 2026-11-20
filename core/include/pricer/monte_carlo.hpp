#pragma once
#include <cstdint>
#include "pricer/types.hpp"

// Monte Carlo under geometric Brownian motion (risk-neutral measure).
namespace pricer::mc {

struct Config {
    std::uint64_t paths = 100'000;   // number of simulated paths (' is a digit separator)
    std::uint64_t seed = 42;         // same seed => same result (reproducible tests)
    bool antithetic = true;          // variance reduction: also simulate -Z for every Z
};

struct Result {
    double price;
    double std_error;   // the true price is within ~2 std_error with 95% confidence
};

// European option: only the terminal value S_T matters, so one draw per path.
Result european(const EuropeanOption& opt, const Market& mkt, const Config& cfg = {});

// Arithmetic-average Asian option: payoff on the average of the spot over
// `steps` equally spaced dates. No closed form exists, hence Monte Carlo.
Result asian_arithmetic(const EuropeanOption& opt, const Market& mkt, int steps,
                        const Config& cfg = {});

}  // namespace pricer::mc
