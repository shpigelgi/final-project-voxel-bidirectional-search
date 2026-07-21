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
# Unique per-submission manifest so a second submission can't clobber the manifest
# that another job's still-pending array tasks read at run time.
export MANIFEST="$CLUSTER_DIR/manifest.$$.tsv"
"$CLUSTER_DIR/make_manifest.sh" "$@"
N=$(wc -l < "$MANIFEST" | tr -d ' ')
if [ "$N" -eq 0 ]; then echo "Empty manifest — nothing to submit." >&2; exit 1; fi

CONC="${CONC:-20}"
mkdir -p "$CLUSTER_DIR/logs"

PARTITION="${PARTITION:-cpu}"   # BGU: dedicated CPU partition (our jobs are CPU-only, no GPU)
extra=(--partition="$PARTITION")
[ -n "${ACCOUNT:-}" ]   && extra+=(--account="$ACCOUNT")

echo "== Submitting array 1-$N%$CONC =="
# Export the config into this shell and pass --export=ALL. Do NOT inline these into
# --export=ALL,VAR=... : --export is itself comma-delimited, so a comma-containing value
# like ALGS=astar,rastar,... would be split and only the first algorithm would survive.
export ROOT="$WS"
export TIMEOUT="${TIMEOUT:-60}"
export MEMMB="${MEMMB:-9000}"
export ALGS="${ALGS:-astar,rastar,mm,bia,bae,nbs,gbfs}"
export LIMIT="${LIMIT:-1000000}"
sbatch --array="1-${N}%${CONC}" --export=ALL "${extra[@]}" "$CLUSTER_DIR/run_array.sbatch"

echo "Submitted. Watch with:  squeue -u \$USER   |   logs in cluster/logs/"
echo "When finished, aggregate with:  cluster/aggregate.py $WS/results"
