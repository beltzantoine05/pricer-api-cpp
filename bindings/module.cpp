// Python bindings: turns the C++ core into a module you can `import pricer`.
// pybind11 converts Python floats/strings/exceptions to C++ and back.
#include <pybind11/pybind11.h>

#include <stdexcept>
#include <string>

#include "pricer/black_scholes.hpp"
#include "pricer/implied_vol.hpp"
#include "pricer/monte_carlo.hpp"

namespace py = pybind11;
using namespace pricer;
using namespace pybind11::literals;  // enables the "name"_a syntax for keyword args

namespace {

// Python callers pass "call"/"put"; std::invalid_argument becomes ValueError.
OptionType parse_type(const std::string& s) {
    if (s == "call") return OptionType::Call;
    if (s == "put") return OptionType::Put;
    throw std::invalid_argument("type must be 'call' or 'put'");
}

}  // namespace

PYBIND11_MODULE(pricer, m) {
    m.doc() = "European option pricer with a C++ core (Black-Scholes, Monte Carlo)";

    py::class_<Greeks>(m, "Greeks")
        .def_readonly("delta", &Greeks::delta)
        .def_readonly("gamma", &Greeks::gamma)
        .def_readonly("vega", &Greeks::vega)
        .def_readonly("theta", &Greeks::theta)
        .def_readonly("rho", &Greeks::rho)
        .def("__repr__", [](const Greeks& g) {
            return "Greeks(delta=" + std::to_string(g.delta) + ", gamma=" +
                   std::to_string(g.gamma) + ", vega=" + std::to_string(g.vega) +
                   ", theta=" + std::to_string(g.theta) + ", rho=" + std::to_string(g.rho) + ")";
        });

    py::class_<mc::Result>(m, "MCResult")
        .def_readonly("price", &mc::Result::price)
        .def_readonly("std_error", &mc::Result::std_error)
        .def("__repr__", [](const mc::Result& r) {
            return "MCResult(price=" + std::to_string(r.price) +
                   ", std_error=" + std::to_string(r.std_error) + ")";
        });

    m.def("bs_price",
          [](const std::string& type, double spot, double strike, double maturity, double rate,
             double vol, double div_yield) {
              return bs::price({parse_type(type), strike, maturity}, {spot, rate, vol, div_yield});
          },
          "type"_a, "spot"_a, "strike"_a, "maturity"_a, "rate"_a, "vol"_a, "div_yield"_a = 0.0,
          "Black-Scholes price of a European option.");

    m.def("bs_greeks",
          [](const std::string& type, double spot, double strike, double maturity, double rate,
             double vol, double div_yield) {
              return bs::greeks({parse_type(type), strike, maturity}, {spot, rate, vol, div_yield});
          },
          "type"_a, "spot"_a, "strike"_a, "maturity"_a, "rate"_a, "vol"_a, "div_yield"_a = 0.0,
          "Analytic Black-Scholes greeks (vega per 1.00 vol, theta per year).");

    m.def("implied_vol",
          [](double price, const std::string& type, double spot, double strike, double maturity,
             double rate, double div_yield) {
              return implied_vol(price, {parse_type(type), strike, maturity},
                                 {spot, rate, 0.0, div_yield});
          },
          "price"_a, "type"_a, "spot"_a, "strike"_a, "maturity"_a, "rate"_a, "div_yield"_a = 0.0,
          "Black-Scholes implied volatility (Newton-Raphson with bisection fallback).");

    m.def("mc_european",
          [](const std::string& type, double spot, double strike, double maturity, double rate,
             double vol, double div_yield, std::uint64_t paths, std::uint64_t seed,
             bool antithetic) {
              py::gil_scoped_release release;  // let other Python threads run meanwhile
              return mc::european({parse_type(type), strike, maturity},
                                  {spot, rate, vol, div_yield}, {paths, seed, antithetic});
          },
          "type"_a, "spot"_a, "strike"_a, "maturity"_a, "rate"_a, "vol"_a, "div_yield"_a = 0.0,
          "paths"_a = 100'000, "seed"_a = 42, "antithetic"_a = true,
          "Monte Carlo price of a European option.");

    m.def("mc_asian",
          [](const std::string& type, double spot, double strike, double maturity, double rate,
             double vol, int steps, double div_yield, std::uint64_t paths, std::uint64_t seed,
             bool antithetic) {
              py::gil_scoped_release release;
              return mc::asian_arithmetic({parse_type(type), strike, maturity},
                                          {spot, rate, vol, div_yield}, steps,
                                          {paths, seed, antithetic});
          },
          "type"_a, "spot"_a, "strike"_a, "maturity"_a, "rate"_a, "vol"_a, "steps"_a = 12,
          "div_yield"_a = 0.0, "paths"_a = 100'000, "seed"_a = 42, "antithetic"_a = true,
          "Monte Carlo price of an arithmetic-average Asian option.");
}
