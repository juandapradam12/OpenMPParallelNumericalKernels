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
scripts/          Benchmark orchestration and plotting
results/          Numeric outputs and timing CSV
figures/          Generated plots
docs/             Case-study notes
```

Responsibilities are separated as follows:

| Layer | Role |
| --- | --- |
| `advection.c` / `random_walk.c` | Domain logic, allocation, OpenMP regions |
| `main_*.c` | Argument parsing, I/O paths, reporting |
| `scripts/run_benchmark.sh` | End-to-end serial/OMP comparison |
| `scripts/plot_results.py` | Result visualization |

Defaults preserve the original experiment sizing so historical results remain comparable, while CLI flags expose grid size, walker count, barrier, seed, and output directory.

## Correctness notes relative to the legacy code

The original flat sources had several issues addressed here:

- Boundary updates used `==` (comparison) instead of assignment.
- OpenMP was applied to loops with loop-carried dependencies (grid construction and time marching).
- Random walks shared a non-thread-safe RNG.
- The full space–time field was retained even though only a few frames were plotted.
- Resources were never released.

## How to read the results

- **Advection plots** should show the initial pulse transported to the right, with numerical diffusion from the first-order upwind scheme. Serial and OpenMP frames should match within floating-point noise.
- **Walk histograms** should be statistically consistent between serial and OpenMP runs when seeds are equivalent.
- **Timing CSV** (`results/timings.csv`) reports wall-clock seconds. Random walks typically show clearer speedup than advection: the ensemble has no time-level dependency and more work per parallel task. On the default advection grid (\(N_x = 1000\)), OpenMP can be slower than serial because thread overhead dominates a tiny inner loop — a useful illustration of when shared-memory parallelism pays off.
