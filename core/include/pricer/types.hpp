#pragma once
// Basic types shared by every pricer.
// A header (.hpp) only *declares* things; the .cpp files hold the code.

namespace pricer {

// `enum class` is a scoped, type-safe enum: you must write OptionType::Call,
// and it cannot be silently converted to an int (unlike a plain C `enum`).
enum class OptionType { Call, Put };

// Market state for one underlying. A `struct` is a class whose members are public.
struct Market {
    double spot;        // S: current price of the underlying
    double rate;        // r: continuously compounded risk-free rate (0.03 = 3%)
    double vol;         // sigma: annualised volatility (0.2 = 20%)
    double div_yield;   // q: continuous dividend yield (0 if none)
};

// A European option: can only be exercised at maturity.
struct EuropeanOption {
    OptionType type;
    double strike;      // K
    double maturity;    // T in years (0.5 = 6 months)
};

// Sensitivities of the price. Units:
//   delta, gamma : per 1 unit of spot
//   vega         : per 1.00 of vol  (divide by 100 for "per vol point")
//   theta        : per year         (divide by 365 for "per day")
//   rho          : per 1.00 of rate (divide by 100 for "per 1%")
struct Greeks {
    double delta;
    double gamma;
    double vega;
    double theta;
    double rho;
};

// Throws std::invalid_argument if an input makes no financial sense.
void validate(const EuropeanOption& opt, const Market& mkt);

}  // namespace pricer
