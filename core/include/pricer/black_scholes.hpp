#pragma once
#include "pricer/types.hpp"

// Closed-form Black-Scholes-Merton for European options, with a dividend yield.
namespace pricer::bs {

// `const T&` = pass by reference without copying, and promise not to modify it.
double price(const EuropeanOption& opt, const Market& mkt);

Greeks greeks(const EuropeanOption& opt, const Market& mkt);

// Standard normal CDF and PDF, exposed because other pricers and tests use them.
double norm_cdf(double x);
double norm_pdf(double x);

}  // namespace pricer::bs
