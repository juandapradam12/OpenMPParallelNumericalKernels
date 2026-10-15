#!/usr/bin/env python3
"""Generate comparison and scaling figures from solver outputs."""

from __future__ import annotations

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "results"
FIGURES = ROOT / "figures"


def latest_snapshots(prefix: str) -> list[Path]:
    files = sorted(RESULTS.glob(f"{prefix}_t*.dat"))
    if not files:
        raise FileNotFoundError(f"No snapshots matching {prefix}_t*.dat")
    return files


def plot_advection() -> None:
    x = np.loadtxt(RESULTS / "advection_x.dat")
    serial_files = latest_snapshots("advection_serial")
    omp_files = latest_snapshots("advection_omp")

    fig, axes = plt.subplots(2, 1, figsize=(8, 7), sharex=True)
    for path in serial_files:
        step = path.stem.split("_t")[-1]
        axes[0].plot(x, np.loadtxt(path), label=f"t-step {int(step)}")
    for path in omp_files:
        step = path.stem.split("_t")[-1]
        axes[1].plot(x, np.loadtxt(path), label=f"t-step {int(step)}")

    axes[0].set_ylabel("u(x)")
    axes[0].set_title("1D advection — serial")
    axes[0].legend(frameon=False, fontsize=8)
    axes[1].set_xlabel("x")
    axes[1].set_ylabel("u(x)")
    axes[1].set_title("1D advection — OpenMP")
    axes[1].legend(frameon=False, fontsize=8)
    fig.tight_layout()
    FIGURES.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIGURES / "advection.png", dpi=160)
    fig.savefig(FIGURES / "advection.pdf")
    plt.close(fig)


def plot_walks() -> None:
    serial = np.loadtxt(RESULTS / "walk_serial.dat")
    omp = np.loadtxt(RESULTS / "walk_omp.dat")

    fig, axes = plt.subplots(2, 1, figsize=(8, 7), sharex=True)
    axes[0].hist(serial, bins=100, color="#1f4e5f", alpha=0.85)
    axes[0].set_ylabel("count")
    axes[0].set_title("Random-walk hitting times — serial")
    axes[1].hist(omp, bins=100, color="#c45c26", alpha=0.85)
    axes[1].set_xlabel("steps to barrier")
    axes[1].set_ylabel("count")
    axes[1].set_title("Random-walk hitting times — OpenMP")
    fig.tight_layout()
    FIGURES.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIGURES / "random_walk.png", dpi=160)
    fig.savefig(FIGURES / "random_walk.pdf")
    plt.close(fig)


def plot_scaling() -> None:
    path = RESULTS / "scaling.csv"
    if not path.exists():
        print("scaling.csv missing; skip scaling figure")
        return

    rows = np.genfromtxt(path, delimiter=",", names=True, dtype=None, encoding="utf-8")
    if rows.size == 0:
        return
    if rows.shape == ():
        rows = np.array([rows])

    problems = sorted(set(rows["problem"].tolist()))
    fig, axes = plt.subplots(1, 2, figsize=(10, 4))

    for problem in problems:
        mask = rows["problem"] == problem
        threads = rows["threads"][mask].astype(float)
        speedup = rows["speedup"][mask].astype(float)
        efficiency = rows["efficiency"][mask].astype(float)
        order = np.argsort(threads)
        threads, speedup, efficiency = threads[order], speedup[order], efficiency[order]
        label = problem.replace("_", " ")
        axes[0].plot(threads, speedup, marker="o", label=label)
        axes[1].plot(threads, efficiency, marker="o", label=label)

    tmax = float(np.max(rows["threads"].astype(float)))
    axes[0].plot([1, tmax], [1, tmax], linestyle="--", color="#888888", label="ideal")
    axes[0].set_xlabel("threads")
    axes[0].set_ylabel("speedup")
    axes[0].set_title("Strong scaling — speedup")
    axes[0].legend(frameon=False, fontsize=8)
    axes[1].axhline(1.0, linestyle="--", color="#888888")
    axes[1].set_xlabel("threads")
    axes[1].set_ylabel("efficiency")
    axes[1].set_title("Strong scaling — efficiency")
    axes[1].set_ylim(0, 1.15)
    axes[1].legend(frameon=False, fontsize=8)
    fig.tight_layout()
    FIGURES.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIGURES / "scaling.png", dpi=160)
    fig.savefig(FIGURES / "scaling.pdf")
    plt.close(fig)


def main() -> None:
    plot_advection()
    plot_walks()
    plot_scaling()
    print(f"Wrote figures under {FIGURES}")


if __name__ == "__main__":
    main()
