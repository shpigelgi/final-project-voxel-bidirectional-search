#!/usr/bin/env bash
#SBATCH --job-name=voxbihs
#SBATCH --output=cluster/logs/%A_%a.out
#SBATCH --error=cluster/logs/%A_%a.out
#SBATCH --time=12:00:00
#SBATCH --mem=10G
#SBATCH --cpus-per-task=1
# --- BGU cluster: set these to match your allocation (or pass via submit.sh) ---
###SBATCH --partition=main
###SBATCH --account=your_account
#
# One array task = one (map, config) unit from cluster/manifest.tsv.
# Reads config from the environment (exported by submit.sh):
#   ROOT     workspace dir      TIMEOUT  per-instance seconds (default 60)
#   MEMMB    per-child MB (900   ALGS     comma list (default all)
#            .. matched to --mem) LIMIT   max instances (default: all)
set -euo pipefail

# module load gcc         # <-- uncomment / adjust for the BGU toolchain
ROOT="${ROOT:?ROOT must be set (workspace dir)}"
TIMEOUT="${TIMEOUT:-60}"
MEMMB="${MEMMB:-9000}"
ALGS="${ALGS:-astar,rastar,mm,bia,bae,nbs,gbfs}"
LIMIT="${LIMIT:-1000000}"

MANIFEST="$ROOT/cluster/manifest.tsv"
BIN="$ROOT/bin"
line=$(sed -n "${SLURM_ARRAY_TASK_ID}p" "$MANIFEST")
IFS=$'\t' read -r family mapf scen cfg <<< "$line"

base="$(basename "$mapf" .3dmap)"
tag="${base}|${cfg}"
diagflag=""; [ "$cfg" = "nodiag" ] && diagflag="--no-diagonals"

outdir="$ROOT/results/$family"
mkdir -p "$outdir" "$ROOT/cluster/logs"

echo "[$(date)] task=$SLURM_ARRAY_TASK_ID family=$family map=$base cfg=$cfg timeout=${TIMEOUT}s mem=${MEMMB}MB"

# Must-expand floor (its own memory can be large on hard maps; same mem budget applies via SLURM).
"$BIN/mvc" "$mapf" "$scen" $diagflag --limit "$LIMIT" \
    > "$outdir/${base}.${cfg}.mvc.csv"

# Algorithm expansions (fork-per-run timeout + memory cap inside the driver).
"$BIN/voxdriver" "$mapf" "$scen" $diagflag \
    --algs "$ALGS" --timeout "$TIMEOUT" --mem-mb "$MEMMB" --limit "$LIMIT" --tag "$tag" \
    > "$outdir/${base}.${cfg}.exp.csv"

echo "[$(date)] done task=$SLURM_ARRAY_TASK_ID"
