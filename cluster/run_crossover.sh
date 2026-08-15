#!/usr/bin/env bash
# Parallelized, representative heuristic-strength crossover re-run.
#
# The original crossover was n=6 on a biased (easy) plant01 prefix, and MM solved
# ~1 instance below w=1.0 - so "MM overtakes A*" rested on a single point. This
# re-runs on a shuffled (representative) sample, more instances, a longer timeout
# (low-w searches explode), and reports per-weight coverage so the medians are real.
#
# Parallelized as one task per (weight, instance-chunk).
set -euo pipefail
CLUSTER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; WS="$(dirname "$CLUSTER_DIR")"
VOX="$WS/benchmarks/voxel-maps"
# MAPS: space-separated map basenames (no extension). Default reproduces the original
# single-map-per-family crossover. To broaden and drop the per-map hedge, pass several
# maps per family, e.g.
#   MAPS="plant01 plant02 plant05 BSG BB CA level01 level07 levels1" ./run_crossover.sh
# Each basename must have both <base>.3dmap and <base>.shuf.3dscen under $VOX
# (run shuffle_scens.py first). Unknown/unshuffled names are skipped with a warning.
MAPS="${MAPS:-plant01}"
WEIGHTS="${WEIGHTS:-1.0 0.8 0.6 0.5 0.4}"
N="${N:-80}"; CHUNK="${CHUNK:-20}"

MAN="$CLUSTER_DIR/manifest.xover.tsv"; : > "$MAN"
NMAPS=0
for base in $MAPS; do
  MAP="$(find "$VOX" -name "${base}.3dmap" | head -1)"
  SCEN="$(find "$VOX" -name "${base}.shuf.3dscen" | head -1)"
  if [ -z "$MAP" ] || [ -z "$SCEN" ]; then
    echo "WARN: skipping '$base' (missing .3dmap or .shuf.3dscen under $VOX)"; continue
  fi
  NMAPS=$((NMAPS+1))
  for hw in $WEIGHTS; do
    s=0; while [ "$s" -lt "$N" ]; do
      printf '%s\t%s\t%s\t%s\t%s\n' "$MAP" "$SCEN" "$hw" "$s" "$CHUNK" >> "$MAN"
      s=$((s+CHUNK))
    done
  done
done
[ "$NMAPS" -eq 0 ] && { echo "no usable maps in MAPS='$MAPS'"; exit 1; }
NT=$(wc -l < "$MAN" | tr -d ' ')
echo "crossover manifest: $NT tasks ($NMAPS maps x $(echo "$WEIGHTS" | wc -w) weights x $((N/CHUNK)) chunks)"

export ROOT="$WS" TIMEOUT="${TIMEOUT:-300}" MEMMB="${MEMMB:-7000}" ALGS=astar,mm,bae,bia,nbs
export MANIFEST="$MAN"
J=$(sbatch --parsable --array="1-${NT}%40" --export=ALL --partition=cpu --mem=8G \
     "$CLUSTER_DIR/crossover_array.sbatch")
echo "CROSSOVER_JOB=$J"
echo "when done: concat_chunks results_crossover/crossover -> aggregate.py results_crossover -> plot_results --crossover"
