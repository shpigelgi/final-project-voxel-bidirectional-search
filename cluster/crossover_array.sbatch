#!/usr/bin/env bash
#SBATCH --job-name=vox-xover
#SBATCH --output=cluster/logs/%A_%a.out
#SBATCH --error=cluster/logs/%A_%a.out
#SBATCH --time=12:00:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=1
# One array task = one (weight, instance-chunk) of the heuristic-strength crossover.
# Manifest columns:  mapf  scen  hweight  start  count
# Writes <base>.w<NNN>.p<start>.{exp,mvc}.csv so a later concat_chunks + aggregate
# yields config=w<NNN> curves. Diagonal (octile) mode only.
set -euo pipefail
ROOT="${ROOT:?}"; TIMEOUT="${TIMEOUT:-300}"; MEMMB="${MEMMB:-7000}"
ALGS="${ALGS:-astar,mm,bae,bia,nbs}"; MANIFEST="${MANIFEST:?}"
BIN="$ROOT/bin"
line=$(sed -n "${SLURM_ARRAY_TASK_ID}p" "$MANIFEST")
IFS=$'\t' read -r mapf scen hw start count <<< "$line"
base="$(basename "$mapf" .3dmap)"
wtag=$(awk "BEGIN{printf \"w%03d\", int($hw*100 + 0.5)}")
suffix="${wtag}.p$(printf '%05d' "$start")"
outdir="$ROOT/results_crossover/crossover"; mkdir -p "$outdir" "$ROOT/cluster/logs"
echo "[$(date)] task=$SLURM_ARRAY_TASK_ID map=$base w=$hw start=$start count=$count timeout=${TIMEOUT}s"
# Floor at this weight (raw expansions are the crossover metric, but keep the floor too).
"$BIN/mvc" "$mapf" "$scen" --hweight "$hw" --start "$start" --limit "$count" \
    > "$outdir/${base}.${suffix}.mvc.csv"
"$BIN/voxdriver" "$mapf" "$scen" --hweight "$hw" --algs "$ALGS" --timeout "$TIMEOUT" \
    --mem-mb "$MEMMB" --start "$start" --limit "$count" --tag "${base}|${wtag}" \
    > "$outdir/${base}.${suffix}.exp.csv"
echo "[$(date)] done task=$SLURM_ARRAY_TASK_ID"
