#include "pricer/monte_carlo.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

namespace pricer::mc {

namespace {

double payoff(OptionType type, double underlying, double strike) {
    return type == OptionType::Call ? std::max(underlying - strike, 0.0)
                                    : std::max(strike - underlying, 0.0);
}

// Running mean and variance in one pass (Welford's algorithm): no need to
// store every sample, and numerically stable.
class RunningStats {
public:
    void add(double x) {
        ++n_;
        const double delta = x - mean_;
        mean_ += delta / static_cast<double>(n_);
        m2_ += delta * (x - mean_);
    }
    double mean() const { return mean_; }
    double std_error() const {
        if (n_ < 2) return 0.0;
        const double n = static_cast<double>(n_);
        return std::sqrt(m2_ / (n - 1.0) / n);
    }

private:
    std::uint64_t n_ = 0;
    double mean_ = 0.0;
    double m2_ = 0.0;
};

void check(const Config& cfg) {
    if (cfg.paths < 2) throw std::invalid_argument("paths must be >= 2");
}

}  // namespace

Result european(const EuropeanOption& opt, const Market& mkt, const Config& cfg) {
    validate(opt, mkt);
    check(cfg);
    std::mt19937_64 rng(cfg.seed);                 // Mersenne Twister generator
    std::normal_distribution<double> normal(0.0, 1.0);

    const double T = opt.maturity;
    const double drift = (mkt.rate - mkt.div_yield - 0.5 * mkt.vol * mkt.vol) * T;
    const double diffusion = mkt.vol * std::sqrt(T);
    const double df = std::exp(-mkt.rate * T);
    auto terminal = [&](double z) { return mkt.spot * std::exp(drift + diffusion * z); };

    RunningStats stats;
    if (cfg.antithetic) {
        // Each sample is the average of a path and its mirror image. Samples
        // must be independent for the std error formula, so we average pairs.
        for (std::uint64_t i = 0; i < cfg.paths / 2; ++i) {
            const double z = normal(rng);
            stats.add(0.5 * (payoff(opt.type, terminal(z), opt.strike) +
                             payoff(opt.type, terminal(-z), opt.strike)));
        }
    } else {
        for (std::uint64_t i = 0; i < cfg.paths; ++i)
            stats.add(payoff(opt.type, terminal(normal(rng)), opt.strike));
    }
    return {df * stats.mean(), df * stats.std_error()};
}

Result asian_arithmetic(const EuropeanOption& opt, const Market& mkt, int steps,
                        const Config& cfg) {
    validate(opt, mkt);
    check(cfg);
    if (steps < 1) throw std::invalid_argument("steps must be >= 1");
    std::mt19937_64 rng(cfg.seed);
    std::normal_distribution<double> normal(0.0, 1.0);

    const double dt = opt.maturity / steps;
    const double drift = (mkt.rate - mkt.div_yield - 0.5 * mkt.vol * mkt.vol) * dt;
    const double diffusion = mkt.vol * std::sqrt(dt);
    const double df = std::exp(-mkt.rate * opt.maturity);

    // Reused between paths to avoid allocating memory in the hot loop.
    std::vector<double> z(static_cast<std::size_t>(steps));

    // Simulates one path from the shocks in z (negated if `mirror`) and
    // returns the payoff on the average spot over the monitoring dates.
    auto path_payoff = [&](bool mirror) {
        double log_s = std::log(mkt.spot), sum = 0.0;
        for (double shock : z) {
            log_s += drift + diffusion * (mirror ? -shock : shock);
            sum += std::exp(log_s);
        }
        return payoff(opt.type, sum / steps, opt.strike);
    };

    RunningStats stats;
    const std::uint64_t n = cfg.antithetic ? cfg.paths / 2 : cfg.paths;
    for (std::uint64_t i = 0; i < n; ++i) {
        for (double& shock : z) shock = normal(rng);  // `double&`: write into the vector
        stats.add(cfg.antithetic ? 0.5 * (path_payoff(false) + path_payoff(true))
                                 : path_payoff(false));
    }
    return {df * stats.mean(), df * stats.std_error()};
}

}  // namespace pricer::mc
