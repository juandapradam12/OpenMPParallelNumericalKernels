#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

SMOKE=0
if [[ "${1:-}" == "--smoke" ]]; then
  SMOKE=1
fi

mkdir -p results results/large figures
# Drop prior numeric outputs so fidelity/smoke grid sizes cannot mix.
rm -f results/*.dat results/large/*.dat results/timings.csv results/scaling.csv
make -s all

NPROC="$(nproc)"
MAX_THREADS="${MAX_THREADS:-$NPROC}"

ADV_POINTS_SMALL=1000
ADV_STEPS_SMALL=700
WALKERS=500000
ADV_POINTS_LARGE=200000
ADV_STEPS_LARGE=400

if [[ "$SMOKE" -eq 1 ]]; then
  ADV_POINTS_SMALL=200
  ADV_STEPS_SMALL=50
  ADV_POINTS_LARGE=20000
  ADV_STEPS_LARGE=80
  WALKERS=20000
  if [[ "$MAX_THREADS" -gt 4 ]]; then
    MAX_THREADS=4
  fi
fi

THREAD_LIST=()
t=1
while [[ "$t" -le "$MAX_THREADS" ]]; do
  THREAD_LIST+=("$t")
  t=$(( t * 2 ))
done
if [[ "${THREAD_LIST[-1]}" -ne "$MAX_THREADS" ]]; then
  THREAD_LIST+=("$MAX_THREADS")
fi

TIMING_CSV="results/timings.csv"
SCALING_CSV="results/scaling.csv"
echo "problem,mode,threads,metric,value" > "$TIMING_CSV"
echo "problem,threads,wall_seconds,speedup,efficiency" > "$SCALING_CSV"

extract_time() {
  sed -n 's/.*time_s=\([0-9.]*\).*/\1/p' | tail -n 1
}

record_timing() {
  echo "$1,$2,$3,wall_seconds,$4" >> "$TIMING_CSV"
}

echo "==> Advection fidelity case (Nx=${ADV_POINTS_SMALL}, Nt=${ADV_STEPS_SMALL})"
ADV_SERIAL_OUT=$(./bin/advection --out results --points "$ADV_POINTS_SMALL" --steps "$ADV_STEPS_SMALL")
echo "$ADV_SERIAL_OUT"
ADV_SERIAL=$(printf '%s\n' "$ADV_SERIAL_OUT" | extract_time)
record_timing "advection_small" serial 1 "$ADV_SERIAL"

export OMP_NUM_THREADS="$MAX_THREADS"
ADV_OMP_OUT=$(./bin/advection --omp --out results --points "$ADV_POINTS_SMALL" --steps "$ADV_STEPS_SMALL")
echo "$ADV_OMP_OUT"
ADV_OMP=$(printf '%s\n' "$ADV_OMP_OUT" | extract_time)
record_timing "advection_small" omp "$MAX_THREADS" "$ADV_OMP"

echo "==> Random walk fidelity case (walkers=${WALKERS})"
unset OMP_NUM_THREADS
WALK_SERIAL_OUT=$(./bin/walk --out results/walk_serial.dat --walkers "$WALKERS")
echo "$WALK_SERIAL_OUT"
WALK_SERIAL=$(printf '%s\n' "$WALK_SERIAL_OUT" | extract_time)
record_timing "random_walk" serial 1 "$WALK_SERIAL"

export OMP_NUM_THREADS="$MAX_THREADS"
WALK_OMP_OUT=$(./bin/walk --omp --out results/walk_omp.dat --walkers "$WALKERS")
echo "$WALK_OMP_OUT"
WALK_OMP=$(printf '%s\n' "$WALK_OMP_OUT" | extract_time)
record_timing "random_walk" omp "$MAX_THREADS" "$WALK_OMP"

echo "==> Strong-scaling sweeps (threads: ${THREAD_LIST[*]})"
ADV_BASE=""
WALK_BASE=""

# Keep CFL ≈ 0.25 when refining the grid (dt scales with dx).
ADV_DT_LARGE=$(python3 -c "print(0.25 * 4.0 / (${ADV_POINTS_LARGE} - 1))")
echo "    large advection dt=${ADV_DT_LARGE} (target CFL≈0.25)"

for threads in "${THREAD_LIST[@]}"; do
  export OMP_NUM_THREADS="$threads"

  if [[ "$threads" -eq 1 ]]; then
    OUT=$(./bin/advection --out results/large --points "$ADV_POINTS_LARGE" --steps "$ADV_STEPS_LARGE" --dt "$ADV_DT_LARGE" --no-snapshots)
  else
    OUT=$(./bin/advection --omp --out results/large --points "$ADV_POINTS_LARGE" --steps "$ADV_STEPS_LARGE" --dt "$ADV_DT_LARGE" --no-snapshots)
  fi
  echo "$OUT"
  SEC=$(printf '%s\n' "$OUT" | extract_time)
  if [[ "$threads" -eq 1 ]]; then
    ADV_BASE="$SEC"
    record_timing "advection_large" serial 1 "$SEC"
  else
    record_timing "advection_large" omp "$threads" "$SEC"
  fi
  SPEEDUP=$(python3 -c "print(float('${ADV_BASE}') / float('${SEC}'))")
  EFF=$(python3 -c "print((float('${ADV_BASE}') / float('${SEC}')) / float('${threads}'))")
  echo "advection_large,${threads},${SEC},${SPEEDUP},${EFF}" >> "$SCALING_CSV"

  if [[ "$threads" -eq 1 ]]; then
    OUT=$(./bin/walk --out results/walk_scale_t1.dat --walkers "$WALKERS")
  else
    OUT=$(./bin/walk --omp --out "results/walk_scale_t${threads}.dat" --walkers "$WALKERS")
  fi
  echo "$OUT"
  SEC=$(printf '%s\n' "$OUT" | extract_time)
  if [[ "$threads" -eq 1 ]]; then
    WALK_BASE="$SEC"
  else
    record_timing "random_walk_scale" omp "$threads" "$SEC"
  fi
  SPEEDUP=$(python3 -c "print(float('${WALK_BASE}') / float('${SEC}'))")
  EFF=$(python3 -c "print((float('${WALK_BASE}') / float('${SEC}')) / float('${threads}'))")
  echo "random_walk,${threads},${SEC},${SPEEDUP},${EFF}" >> "$SCALING_CSV"
done

echo "==> Correctness checks"
python3 scripts/check_correctness.py --steps "$ADV_STEPS_SMALL"

echo "==> Plotting"
python3 scripts/plot_results.py

if [[ "$SMOKE" -eq 0 ]]; then
  cp "$TIMING_CSV" results/timings.example.csv
  cp "$SCALING_CSV" results/scaling.example.csv
fi

echo "==> Done"
echo "--- timings ---"
cat "$TIMING_CSV"
echo "--- scaling ---"
cat "$SCALING_CSV"
