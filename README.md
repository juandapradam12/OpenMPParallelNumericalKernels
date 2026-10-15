# Parallel Numerical Kernels

**1D advection (finite differences) and Monte Carlo random walks with OpenMP**

Portfolio case study in shared-memory parallel scientific computing: two complementary kernels, a small C11 library layout, serial-vs-OpenMP correctness checks, strong-scaling measurements, and visualization of physical / statistical outputs.

> Suggested GitHub repository name: `parallel-numerical-kernels`  
> (the current remote may still use the legacy homework name; rename under GitHub → Settings → General.)

---

## Use case

| Kernel | Method | Parallel strategy |
| --- | --- | --- |
| Linear advection \(\partial_t u + c\partial_x u = 0\) | First-order upwind FD | OpenMP over space; time remains sequential |
| Absorbing-barrier random walk | Monte Carlo ensemble | OpenMP over independent walkers + per-walker RNG |

The default fidelity case keeps the original study sizing (\(N_x = 1000\), \(N_t = 700\), \(5\times10^5\) walkers). A second **large-grid advection** case (\(N_x = 2\times10^5\), \(N_t = 400\)) is used for strong scaling, where OpenMP has enough work per step to beat serial.

## Project layout

```
include/     Public APIs (advection, random walk, timing)
src/         Solver implementations and CLI entry points
scripts/     Benchmark, correctness checks, plotting
docs/        Case-study write-up (methods, architecture, results)
results/     Generated data, timings, scaling CSVs
figures/     Generated plots
.github/     CI smoke workflow
```

Details: [docs/case-study.md](docs/case-study.md).

## Requirements

- GCC with OpenMP (`-fopenmp`)
- Make
- Python 3 with `numpy` and `matplotlib` (see `requirements.txt`)

## Build and run

```bash
make
./scripts/run_benchmark.sh          # full fidelity + scaling study
./scripts/run_benchmark.sh --smoke  # short CI-friendly run
```

Or invoke solvers directly:

```bash
./bin/advection                       # serial fidelity defaults
./bin/advection --omp                 # OpenMP spatial update
./bin/advection --points 200000 --steps 400 --no-snapshots --omp
./bin/walk --omp --walkers 500000
```

Useful flags: `--points`, `--steps`, `--dt`, `--walkers`, `--barrier`, `--seed`, `--out`, `--no-snapshots`.

```bash
MAX_THREADS=8 ./scripts/run_benchmark.sh
```

Outputs: `results/timings.csv`, `results/scaling.csv`, snapshots, and figures under `figures/`.

## Sample outputs

Pulse transport (serial vs OpenMP agree to machine precision):

![Advection serial vs OpenMP](figures/advection.png)

Hitting-time histograms for \(5\times10^5\) walkers:

![Random walk serial vs OpenMP](figures/random_walk.png)

Strong scaling (speedup / efficiency):

![Strong scaling](figures/scaling.png)

Example numbers are committed in `results/timings.example.csv` and `results/scaling.example.csv`.

## What this demonstrates

- Separating numerical cores from CLI / I/O
- Correct OpenMP placement (no parallelization of dependent time loops)
- Thread-safe Monte Carlo seeding with `erand48`
- Double-buffered PDE integration and I/O-excluded wall timing
- When parallelism helps (large advection / walkers) vs when overhead wins (tiny grids)
- Reproducible smoke CI on every push

## License

[MIT](LICENSE) © Juan David Prada Malagon
