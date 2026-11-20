# pricer-cpp

A small option pricing library with a **C++17 core**, exposed to **Python** through pybind11 and served as a **REST API** with FastAPI. It mirrors how bank pricing libraries are organised: C++ computes, other languages consume.

```
core/       C++17 library: Black-Scholes, Greeks, implied vol, Monte Carlo
tests/      GoogleTest suite (12 tests)
bindings/   pybind11 module -> `import pricer`
api/        FastAPI service + pytest suite (8 tests)
bench/      C++ vs NumPy vs pure Python benchmark
```

## Features

| Feature | Method |
|---|---|
| European call/put price | Black-Scholes-Merton closed form, with continuous dividend yield |
| Greeks | Analytic delta, gamma, vega, theta, rho |
| Implied volatility | Newton-Raphson on vega, safeguarded by bisection; rejects prices outside no-arbitrage bounds |
| European Monte Carlo | Exact GBM terminal draw, antithetic variates, standard error |
| Asian (arithmetic average) Monte Carlo | Path simulation over N monitoring dates, antithetic variates |

## How it is tested

- **Reference values** from Hull (ATM call 10.4506, put 5.5735).
- **Put-call parity** across strikes and maturities, to 1e-10.
- **Every analytic Greek checked against a central finite difference** of the price, for calls and puts.
- **Implied vol round trip** over strikes from 60 to 160 and vols from 5% to 150%.
- **Monte Carlo converges to the closed form** within 4 standard errors; antithetic variates reduce the standard error; same seed gives the same result; an Asian with one date equals the European.
- **Input validation**: invalid spot, vol, maturity or NaN throw `std::invalid_argument`, surfaced as `ValueError` in Python and HTTP 422 in the API.

## Build and run

Requirements: a C++17 compiler (GCC or Clang) and Python 3.10+.

```bash
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt

cmake -S . -B build -G Ninja -DPython_EXECUTABLE="$(which python)" \
      -Dpybind11_DIR="$(python -m pybind11 --cmakedir)"
cmake --build build

ctest --test-dir build --output-on-failure   # C++ tests
python -m pytest -q api                      # API tests
```

### From Python

```python
import sys; sys.path.insert(0, "python")
import pricer

pricer.bs_price("call", spot=100, strike=100, maturity=1, rate=0.05, vol=0.2)   # 10.4506
pricer.bs_greeks("put", 100, 100, 1, 0.05, 0.2)
pricer.implied_vol(10.4506, "call", 100, 100, 1, 0.05)                         # 0.2000
pricer.mc_asian("call", 100, 100, 1, 0.05, 0.2, steps=12, paths=500_000)
```

### As an API

```bash
uvicorn api.main:app --reload     # interactive docs at http://127.0.0.1:8000/docs
curl -X POST localhost:8000/bs/price -H 'Content-Type: application/json' \
     -d '{"type":"call","spot":100,"strike":100,"maturity":1,"rate":0.05,"vol":0.2}'
```

Endpoints: `GET /health`, `POST /bs/price`, `/bs/greeks`, `/bs/implied-vol`, `/mc/european`, `/mc/asian`.

## Benchmark

European call, 2,000,000 antithetic paths (`python bench/bench_mc.py`):

| Engine | Time | vs C++ |
|---|---|---|
| C++ (pybind11) | 0.034 s | 1.0x |
| NumPy (vectorised) | 0.037 s | 1.1x |
| Pure Python loop | 0.53 s (extrapolated) | ~16x |

For a single-draw European payoff, vectorised NumPy is already close to C++. The C++ core pays off on path-dependent products and loops that do not vectorise, and it is what production pricing libraries are written in.

## Conventions

Vega is per 1.00 of volatility (divide by 100 for one vol point), theta is per year (divide by 365 for one day), rho is per 1.00 of rate.

## Roadmap

- [ ] American options (binomial tree, then Longstaff-Schwartz)
- [ ] Barrier options
- [ ] Control variate for the Asian (geometric Asian closed form)
- [ ] Multithreaded Monte Carlo
- [ ] Excel add-in via xlwings
