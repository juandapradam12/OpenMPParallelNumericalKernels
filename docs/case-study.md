# Case Study: Shared-Memory Parallelism for Numerical Kernels

## Problem

Two workloads appear frequently in scientific computing and quantitative modeling:

1. **Transport PDEs** — evolve a field under a hyperbolic conservation law.
2. **Monte Carlo ensembles** — aggregate many independent stochastic trajectories.

Both are natural targets for shared-memory parallelism, but they expose different constraints. The advection update is sequential in time and only parallel across space. Random walks are embarrassingly parallel across walkers, yet require a thread-safe random-number generator.

This repository implements both kernels in C11 with OpenMP, measures serial vs. parallel wall time, and visualizes the outputs.

## Numerical methods

### 1D linear advection

The model equation is

\[
\partial_t u + c\,\partial_x u = 0, \quad c > 0
\]

on \(x \in [-2, 2]\), with a compact initial pulse

\[
u(x,0) =
\begin{cases}
0.25 - x^2 & |x| \le 0.5 \\
0 & \text{otherwise}
\end{cases}
\]

and fixed Dirichlet boundaries. Time integration uses a first-order upwind finite-difference scheme:

\[
u^{n+1}_i = u^n_i - \alpha\,(u^n_i - u^n_{i-1}), \quad \alpha = c\,\Delta t / \Delta x
\]

Stability requires the CFL condition \(\alpha \le 1\). The default grid yields \(\alpha \approx 0.25\).

**Memory design.** Instead of allocating a full space–time array, the solver keeps two spatial buffers and swaps them each step. Selected snapshots are written on demand, which lowers memory from \(O(N_t N_x)\) to \(O(N_x)\).

**Parallelism.** OpenMP parallelizes the inner spatial loop only. Parallelizing the time loop would be incorrect because \(u^{n+1}\) depends on \(u^n\).

### Absorbing-barrier random walk

Each walker starts at the origin and takes unit-bounded steps

\[
x_{k+1} = x_k + 2\,(U_k - 1/2), \quad U_k \sim \mathcal{U}(0,1)
\]

until \(|x| \ge B\) (default \(B = 10\)). The recorded observable is the hitting time (number of steps). An ensemble of \(5 \times 10^5\) walkers produces the empirical distribution of hitting times.

**Parallelism.** Walkers are independent, so the ensemble loop is distributed with OpenMP `static` scheduling. Each walker owns a private `erand48` seed state, avoiding the race conditions of shared `drand48`/`srand48`.

## Architecture

```
include/          Public headers (config, state, API)
src/              Solver cores + thin CLI entry points
scripts/          Benchmark, correctness checks, plotting
results/          Numeric outputs, timings, scaling CSVs
figures/          Generated plots
.github/          CI smoke workflow
docs/             Case-study notes
```

Responsibilities are separated as follows:

| Layer | Role |
| --- | --- |
| `advection.c` / `random_walk.c` | Domain logic, allocation, OpenMP regions |
| `main_*.c` | Argument parsing, I/O paths, reporting |
| `scripts/run_benchmark.sh` | Fidelity run + strong-scaling sweep |
| `scripts/check_correctness.py` | Serial/OMP equality + peak-position smoke check |
| `scripts/plot_results.py` | Field, histogram, and scaling figures |

Defaults preserve the original experiment sizing for fidelity. Scaling uses a larger advection grid and a thread sweep (`1, 2, 4, …` up to `MAX_THREADS`).

## Correctness notes relative to the legacy code

The original flat sources had several issues addressed here:

- Boundary updates used `==` (comparison) instead of assignment.
- OpenMP was applied to loops with loop-carried dependencies (grid construction and time marching).
- Random walks shared a non-thread-safe RNG.
- The full space–time field was retained even though only a few frames were plotted.
- Resources were never released.

Automated checks assert bit-identical serial/OpenMP outputs for both kernels (same seeds) and that the advected peak sits near \(x \approx c\,t\).

## Experiments

| Experiment | Purpose | Size |
| --- | --- | --- |
| `advection_small` | Visual fidelity + overhead demo | \(N_x=1000\), \(N_t=700\) |
| `advection_large` | Strong scaling where OpenMP helps | \(N_x=2\times10^5\), \(N_t=400\) |
| `random_walk` | Histogram fidelity + scaling | \(5\times10^5\) walkers |

## Example results

Representative numbers from a 4-thread run (`results/scaling.example.csv`; machine-dependent):

| Problem | Threads | Wall time (s) | Speedup | Efficiency |
| --- | --- | --- | --- | --- |
| random_walk | 1 | 0.673 | 1.00 | 1.00 |
| random_walk | 2 | 0.340 | 1.98 | 0.99 |
| random_walk | 4 | 0.171 | 3.94 | 0.99 |
| advection_large | 1 | 0.040 | 1.00 | 1.00 |
| advection_large | 2 | 0.019 | 2.06 | 1.03 |
| advection_large | 4 | 0.010 | 4.04 | 1.01 |

The small fidelity advection grid (\(N_x=1000\)) is intentionally kept: on the same host it was **slower** under OpenMP (≈0.0002 s serial vs ≈0.0015 s on 4 threads), which is the overhead lesson next to the large-grid win. Large-grid runs scale \(\Delta t\) with \(\Delta x\) so the CFL number stays ≈ 0.25.

## How to read the results

- **Advection plots** should show the initial pulse transported to the right, with numerical diffusion from the first-order upwind scheme. Serial and OpenMP frames should match within floating-point noise.
- **Walk histograms** should match exactly between serial and OpenMP when walker seeds are equivalent.
- **Scaling figure** (`figures/scaling.png`) shows speedup and parallel efficiency versus thread count. Random walks scale cleanly; large-grid advection improves once the spatial loop is large enough; the small fidelity grid often slows down under OpenMP because thread startup exceeds useful work.
