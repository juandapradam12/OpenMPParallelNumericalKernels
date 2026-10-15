#!/usr/bin/env python3
"""Smoke correctness checks for serial vs OpenMP outputs."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "results"


def max_abs(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.max(np.abs(a - b)))


def check_advection(n_steps: int = 700, c: float = 1.0, dt: float = 1e-3) -> None:
    x = np.loadtxt(RESULTS / "advection_x.dat")
    serial = sorted(RESULTS.glob("advection_serial_t*.dat"))
    omp = sorted(RESULTS.glob("advection_omp_t*.dat"))
    if not serial or not omp:
        raise FileNotFoundError("Missing advection snapshots under results/")
    if len(serial) != len(omp):
        raise AssertionError("Serial/OpenMP snapshot counts differ")

    for s_path, o_path in zip(serial, omp):
        err = max_abs(np.loadtxt(s_path), np.loadtxt(o_path))
        if err != 0.0:
            raise AssertionError(f"Advection mismatch at {s_path.name}: max_abs={err}")

    final_path = serial[-1]
    final = np.loadtxt(final_path)
    if final.shape != x.shape:
        raise AssertionError(
            f"Grid/field size mismatch: x={x.shape}, {final_path.name}={final.shape}"
        )
    peak_x = float(x[int(np.argmax(final))])
    expected = c * dt * n_steps
    # First-order upwind is dissipative; allow a modest lag relative to the domain.
    # Smoke runs use a short horizon; keep the tolerance relative to travel distance.
    tol = max(0.25, 0.35 * max(expected, 1e-6))
    if abs(peak_x - expected) > tol:
        raise AssertionError(
            f"Advection peak at x={peak_x:.4f}, expected near {expected:.4f} (±{tol})"
        )
    print(f"advection ok: serial==omp, peak_x={peak_x:.4f} (analytic ~{expected:.4f})")


def check_walks() -> None:
    serial = np.loadtxt(RESULTS / "walk_serial.dat")
    omp = np.loadtxt(RESULTS / "walk_omp.dat")
    if serial.shape != omp.shape:
        raise AssertionError("Walk ensemble sizes differ")
    err = max_abs(serial, omp)
    if err != 0.0:
        raise AssertionError(f"Walk mismatch: max_abs={err}")
    print(f"random_walk ok: serial==omp, n={serial.size}, mean={serial.mean():.3f}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--steps", type=int, default=700, help="advection time steps")
    args = parser.parse_args()
    check_advection(n_steps=args.steps)
    check_walks()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # noqa: BLE001 - surface as CI failure
        print(f"CORRECTNESS FAILED: {exc}", file=sys.stderr)
        raise SystemExit(1)
