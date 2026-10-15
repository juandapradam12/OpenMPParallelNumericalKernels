#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mkdir -p results figures
make -s all

export OMP_NUM_THREADS="${OMP_NUM_THREADS:-$(nproc)}"

TIMING_CSV="results/timings.csv"
echo "problem,mode,threads,metric,value" > "$TIMING_CSV"

extract_time() {
  # Input line example: ... time_s=0.123456
  sed -n 's/.*time_s=\([0-9.]*\).*/\1/p' | tail -n 1
}

echo "==> Advection (serial)"
ADV_SERIAL_OUT=$(./bin/advection --out results)
echo "$ADV_SERIAL_OUT"
ADV_SERIAL=$(printf '%s\n' "$ADV_SERIAL_OUT" | extract_time)
echo "advection,serial,1,wall_seconds,${ADV_SERIAL}" >> "$TIMING_CSV"

echo "==> Advection (OpenMP, threads=${OMP_NUM_THREADS})"
ADV_OMP_OUT=$(./bin/advection --omp --out results)
echo "$ADV_OMP_OUT"
ADV_OMP=$(printf '%s\n' "$ADV_OMP_OUT" | extract_time)
echo "advection,omp,${OMP_NUM_THREADS},wall_seconds,${ADV_OMP}" >> "$TIMING_CSV"

echo "==> Random walk (serial)"
WALK_SERIAL_OUT=$(./bin/walk --out results/walk_serial.dat)
echo "$WALK_SERIAL_OUT"
WALK_SERIAL=$(printf '%s\n' "$WALK_SERIAL_OUT" | extract_time)
echo "random_walk,serial,1,wall_seconds,${WALK_SERIAL}" >> "$TIMING_CSV"

echo "==> Random walk (OpenMP, threads=${OMP_NUM_THREADS})"
WALK_OMP_OUT=$(./bin/walk --omp --out results/walk_omp.dat)
echo "$WALK_OMP_OUT"
WALK_OMP=$(printf '%s\n' "$WALK_OMP_OUT" | extract_time)
echo "random_walk,omp,${OMP_NUM_THREADS},wall_seconds,${WALK_OMP}" >> "$TIMING_CSV"

if command -v python3 >/dev/null 2>&1; then
  echo "==> Plotting"
  python3 scripts/plot_results.py
fi

echo "==> Done. Timings in ${TIMING_CSV}"
cat "$TIMING_CSV"
