# Parallel Numerical Kernels

**1D advection (finite differences) and Monte Carlo random walks with OpenMP**

Portfolio case study in shared-memory parallel scientific computing: two complementary kernels, a small C11 library layout, reproducible serial-vs-OpenMP benchmarks, and visualization of the physical / statistical outputs.

---

## Use case

| Kernel | Method | Parallel strategy |
| --- | --- | --- |
| Linear advection \(\partial_t u + c\partial_x u = 0\) | First-order upwind FD | OpenMP over space; time remains sequential |
| Absorbing-barrier random walk | Monte Carlo ensemble | OpenMP over independent walkers + per-walker RNG |

Default experiment sizes match the original study (\(N_x = 1000\), \(N_t = 700\), \(5\times10^5\) walkers) so the case remains comparable while the implementation is production-shaped: clear modules, CLI configuration, CFL awareness, and measured wall time.

## Project layout

```
include/     Public APIs (advection, random walk, timing)
src/         Solver implementations and CLI entry points
scripts/     Benchmark runner and plotting
docs/        Case-study write-up (methods, architecture, results)
results/     Generated data and timings.csv
figures/     Generated plots
```

Details on numerics, parallelism constraints, and design choices: [docs/case-study.md](docs/case-study.md).

## Requirements

- GCC with OpenMP (`-fopenmp`)
- Make
- Python 3 with `numpy` and `matplotlib` (see `requirements.txt`)

## Build and run

```bash
make
./scripts/run_benchmark.sh
```

Or invoke solvers directly:

```bash
./bin/advection            # serial
./bin/advection --omp      # OpenMP spatial update
./bin/walk                 # serial ensemble
./bin/walk --omp           # OpenMP ensemble
```

Useful flags: `--points`, `--steps`, `--walkers`, `--barrier`, `--seed`, `--out`.

Thread count follows the usual OpenMP environment variable:

```bash
OMP_NUM_THREADS=4 ./scripts/run_benchmark.sh
```

Outputs land in `results/` (field snapshots, walk samples, `timings.csv`) and `figures/` (`advection.png`, `random_walk.png`).

## Sample outputs

Pulse transport under the upwind scheme (serial vs OpenMP frames agree to machine precision on this grid):

![Advection serial vs OpenMP](figures/advection.png)

Hitting-time histograms for \(5\times10^5\) walkers:

![Random walk serial vs OpenMP](figures/random_walk.png)

Example wall-clock comparison on 4 threads (`results/timings.example.csv`): random-walk ensemble ≈ **4×** faster with OpenMP; advection at \(N_x=1000\) shows thread overhead dominating — see the [case study](docs/case-study.md).

## What this demonstrates

- Separating numerical cores from CLI / I/O
- Correct OpenMP placement (no parallelization of dependent time loops)
- Thread-safe Monte Carlo seeding with `erand48`
- Double-buffered PDE integration instead of a full space–time tensor
- Lightweight benchmarking suitable for a portfolio narrative

## License

Personal portfolio project. Use and adaptation welcome with attribution.
