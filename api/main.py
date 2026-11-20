"""REST API exposing the C++ pricer. Run: uvicorn api.main:app --reload"""

import sys
from pathlib import Path
from typing import Literal

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field

# The compiled module is written to ./python by CMake.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "python"))
import pricer  # noqa: E402

app = FastAPI(
    title="pricer-cpp",
    description="European and Asian option pricing backed by a C++ core.",
    version="0.1.0",
)


class OptionIn(BaseModel):
    type: Literal["call", "put"]
    spot: float = Field(gt=0, examples=[100.0])
    strike: float = Field(gt=0, examples=[100.0])
    maturity: float = Field(gt=0, description="years", examples=[1.0])
    rate: float = Field(examples=[0.05])
    div_yield: float = 0.0


class PricedOptionIn(OptionIn):
    vol: float = Field(gt=0, examples=[0.2])


class ImpliedVolIn(OptionIn):
    price: float = Field(gt=0, examples=[10.45])


class MonteCarloIn(PricedOptionIn):
    paths: int = Field(100_000, ge=2, le=10_000_000)
    seed: int = Field(42, ge=0)
    antithetic: bool = True


class AsianIn(MonteCarloIn):
    steps: int = Field(12, ge=1, le=1_000)


def _call(fn, **kwargs):
    """C++ std::invalid_argument arrives as ValueError: return a clean 422."""
    try:
        return fn(**kwargs)
    except ValueError as exc:
        raise HTTPException(status_code=422, detail=str(exc)) from exc


@app.get("/health")
def health():
    return {"status": "ok"}


@app.post("/bs/price")
def bs_price(o: PricedOptionIn):
    return {"price": _call(pricer.bs_price, **o.model_dump())}


@app.post("/bs/greeks")
def bs_greeks(o: PricedOptionIn):
    g = _call(pricer.bs_greeks, **o.model_dump())
    return {k: getattr(g, k) for k in ("delta", "gamma", "vega", "theta", "rho")}


@app.post("/bs/implied-vol")
def implied_vol(o: ImpliedVolIn):
    return {"implied_vol": _call(pricer.implied_vol, **o.model_dump())}


@app.post("/mc/european")
def mc_european(o: MonteCarloIn):
    r = _call(pricer.mc_european, **o.model_dump())
    return {"price": r.price, "std_error": r.std_error}


@app.post("/mc/asian")
def mc_asian(o: AsianIn):
    r = _call(pricer.mc_asian, **o.model_dump())
    return {"price": r.price, "std_error": r.std_error}
