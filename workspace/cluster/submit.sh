#!/usr/bin/env bash
# Build the binaries, generate the manifest, and submit the Slurm array.
#
#   ./submit.sh [family ...]
#
# Tunables (env vars):
#   TIMEOUT  per-instance wall-clock seconds (default 60)
#   MEMMB    per-child memory cap in MB      (default 9000; keep < SBATCH --mem)
#   ALGS     comma-separated algorithms      (default all six)
#   LIMIT    max instances per map           (default all)
#   CONC     max concurrent array tasks      (default 20)
#   PARTITION / ACCOUNT  passed through to sbatch if set
set -euo pipefail

CLUSTER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS="$(dirname "$CLUSTER_DIR")"

echo "== Building binaries into $WS/bin =="
mkdir -p "$WS/bin"
"$WS/src/build.sh" "$WS/bin"

echo "== Generating manifest =="
"$CLUSTER_DIR/make_manifest.sh" "$@"
N=$(wc -l < "$CLUSTER_DIR/manifest.tsv" | tr -d ' ')
if [ "$N" -eq 0 ]; then echo "Empty manifest — nothing to submit." >&2; exit 1; fi

CONC="${CONC:-20}"
mkdir -p "$CLUSTER_DIR/logs"

PARTITION="${PARTITION:-cpu}"   # BGU: dedicated CPU partition (our jobs are CPU-only, no GPU)
extra=(--partition="$PARTITION")
[ -n "${ACCOUNT:-}" ]   && extra+=(--account="$ACCOUNT")

echo "== Submitting array 1-$N%$CONC =="
sbatch --array="1-${N}%${CONC}" \
       --export=ALL,ROOT="$WS",TIMEOUT="${TIMEOUT:-60}",MEMMB="${MEMMB:-9000}",ALGS="${ALGS:-astar,rastar,mm,bia,bae,nbs,gbfs}",LIMIT="${LIMIT:-1000000}" \
       "${extra[@]}" \
       "$CLUSTER_DIR/run_array.sbatch"

echo "Submitted. Watch with:  squeue -u \$USER   |   logs in cluster/logs/"
echo "When finished, aggregate with:  cluster/aggregate.py $WS/results"
