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

MANIFEST="${MANIFEST:-$ROOT/cluster/manifest.tsv}"
BIN="$ROOT/bin"
line=$(sed -n "${SLURM_ARRAY_TASK_ID}p" "$MANIFEST")
# Manifest columns: family  mapf  scen  config  [start  count]
# The optional 5th/6th columns chunk one (map,config) into an instance range
# [start, start+count) so a slow map can be split across parallel tasks. When
# absent, the whole map runs (legacy 4-column manifest) and filenames are
# unchanged; when present, each chunk writes a distinct .pNNNNN file that a
# post-run concat step stitches back into the canonical base.cfg.{exp,mvc}.csv.
IFS=$'\t' read -r family mapf scen cfg start count <<< "$line"

base="$(basename "$mapf" .3dmap)"
tag="${base}|${cfg}"
diagflag=""; [ "$cfg" = "nodiag" ] && diagflag="--no-diagonals"

if [ -n "${start:-}" ]; then
    suffix=".p$(printf '%05d' "$start")"        # chunked
else
    suffix=""; start=0; count="$LIMIT"          # whole map (legacy)
fi

outdir="$ROOT/results/$family"
mkdir -p "$outdir" "$ROOT/cluster/logs"

echo "[$(date)] task=$SLURM_ARRAY_TASK_ID family=$family map=$base cfg=$cfg start=$start count=$count timeout=${TIMEOUT}s mem=${MEMMB}MB"

# Must-expand floor (its own memory can be large on hard maps; same mem budget applies via SLURM).
"$BIN/mvc" "$mapf" "$scen" $diagflag --start "$start" --limit "$count" \
    > "$outdir/${base}.${cfg}${suffix}.mvc.csv"

# Algorithm expansions (fork-per-run timeout + memory cap inside the driver).
"$BIN/voxdriver" "$mapf" "$scen" $diagflag \
    --algs "$ALGS" --timeout "$TIMEOUT" --mem-mb "$MEMMB" --start "$start" --limit "$count" --tag "$tag" \
    > "$outdir/${base}.${cfg}${suffix}.exp.csv"

echo "[$(date)] done task=$SLURM_ARRAY_TASK_ID"
