"""Compare the C++ Monte Carlo with a vectorised NumPy version and a pure Python loop.

Run from the repo root: python bench/bench_mc.py
"""

import math
import sys
import time
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "python"))
import pricer  # noqa: E402

S, K, T, R, VOL = 100.0, 100.0, 1.0, 0.05, 0.2
PATHS = 2_000_000


def numpy_mc(paths: int, seed: int = 42) -> float:
    z = np.random.default_rng(seed).standard_normal(paths // 2)
    z = np.concatenate([z, -z])  # antithetic
    st = S * np.exp((R - 0.5 * VOL**2) * T + VOL * math.sqrt(T) * z)
    return math.exp(-R * T) * np.maximum(st - K, 0.0).mean()


def python_mc(paths: int, seed: int = 42) -> float:
    import random

    rng = random.Random(seed)
    drift, diff, total = (R - 0.5 * VOL**2) * T, VOL * math.sqrt(T), 0.0
    for _ in range(paths):
        total += max(S * math.exp(drift + diff * rng.gauss(0.0, 1.0)) - K, 0.0)
    return math.exp(-R * T) * total / paths


def timed(fn, *args, repeat: int = 3):
    best = float("inf")
    for _ in range(repeat):
        t0 = time.perf_counter()
        out = fn(*args)
        best = min(best, time.perf_counter() - t0)
    return out, best


if __name__ == "__main__":
    exact = pricer.bs_price("call", S, K, T, R, VOL)
    cpp, t_cpp = timed(lambda n: pricer.mc_european("call", S, K, T, R, VOL, paths=n).price, PATHS)
    npy, t_npy = timed(numpy_mc, PATHS)
    py, t_py = timed(python_mc, PATHS // 10, repeat=1)
    t_py *= 10  # extrapolated to the same number of paths

    print(f"Closed form            : {exact:.4f}")
    print(f"{'Engine':<22} {'Price':>8} {'Time (s)':>10} {'vs C++':>8}")
    for name, px, t in [("C++ (pybind11)", cpp, t_cpp), ("NumPy (vectorised)", npy, t_npy),
                        ("Pure Python (extrap.)", py, t_py)]:
        print(f"{name:<22} {px:>8.4f} {t:>10.3f} {t / t_cpp:>7.1f}x")
