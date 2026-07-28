#!/usr/bin/env bash
#SBATCH --job-name=voxxover
#SBATCH --output=cluster/logs/xover_%j.out
#SBATCH --error=cluster/logs/xover_%j.out
#SBATCH --time=6:00:00
#SBATCH --mem=10G
#SBATCH --cpus-per-task=1
#
# Heuristic-strength crossover study: run one map at several heuristic weights
# w (h' = w*h, still admissible) and record expansions + the matching MVC floor
# per weight, for every optimal algorithm. As w -> 0 the heuristic weakens toward
# Dijkstra; the point is to see whether/when bidirectional search overtakes A*.
#
# Env: ROOT, MAP, SCEN, WEIGHTS (space list), LIMIT, TIMEOUT, MEMMB, ALGS
set -uo pipefail
ROOT="${ROOT:?ROOT (workspace dir) required}"; BIN="$ROOT/bin"
MAP="${MAP:?}"; SCEN="${SCEN:?}"; BASE=$(basename "$MAP" .3dmap)
WEIGHTS="${WEIGHTS:-1.0 0.75 0.5 0.25 0.1 0.0}"
START="${START:-0}"                 # skip trivial low-index instances
LIMIT="${LIMIT:-40}"; TIMEOUT="${TIMEOUT:-90}"; MEMMB="${MEMMB:-8000}"
ALGS="${ALGS:-astar,rastar,mm,bia,bae,nbs}"
OUT="$ROOT/results_crossover/crossover"; mkdir -p "$OUT" "$ROOT/cluster/logs"

for w in $WEIGHTS; do
  wtag=$(awk -v w="$w" 'BEGIN{printf "w%03d", w*100}')   # 1.0->w100, 0.5->w050, 0.0->w000
  echo "[$(date)] weight=$w tag=$wtag"
  "$BIN/mvc"      "$MAP" "$SCEN" --hweight "$w" --start "$START" --limit "$LIMIT" \
      > "$OUT/$BASE.$wtag.mvc.csv"
  "$BIN/voxdriver" "$MAP" "$SCEN" --hweight "$w" --start "$START" --algs "$ALGS" \
      --timeout "$TIMEOUT" --mem-mb "$MEMMB" --limit "$LIMIT" --tag "$BASE|$wtag" \
      > "$OUT/$BASE.$wtag.exp.csv"
done
echo "XOVER_DONE $(date)"
