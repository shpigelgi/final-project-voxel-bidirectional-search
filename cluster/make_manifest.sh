#!/usr/bin/env bash
# Build a job manifest: one line per (map, config) experiment unit.
# Each map is paired with its scenario by basename; each map runs in two
# configs: with diagonals (26-connected) and without (6-connected).
#
#   ./make_manifest.sh [family ...]     # default: every family present in benchmarks/
#
# Output (TSV, to cluster/manifest.tsv):  family  map_path  scen_path  config
set -euo pipefail

CLUSTER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS="$(dirname "$CLUSTER_DIR")"
VOX="$WS/benchmarks/voxel-maps"
# Per-submission manifest (set by submit.sh) so concurrent submissions of different
# families don't clobber each other — array tasks read their manifest at RUN time.
OUT="${MANIFEST:-$CLUSTER_DIR/manifest.tsv}"

if [ ! -d "$VOX" ]; then echo "No benchmarks at $VOX — run scripts/fetch-benchmarks.sh first." >&2; exit 1; fi

FAMILIES=("$@")
if [ ${#FAMILIES[@]} -eq 0 ]; then
  FAMILIES=()
  for d in "$VOX"/*/; do FAMILIES+=("$(basename "$d")"); done
fi

: > "$OUT"
for fam in "${FAMILIES[@]}"; do
  while IFS= read -r mapf; do
    base="$(basename "$mapf" .3dmap)"
    scen="$(find "$VOX/$fam" -name "$base.3dscen" | head -1)"
    if [ -z "$scen" ]; then echo "WARN: no scenario for $mapf" >&2; continue; fi
    for cfg in diag nodiag; do
      printf "%s\t%s\t%s\t%s\n" "$fam" "$mapf" "$scen" "$cfg" >> "$OUT"
    done
  done < <(find "$VOX/$fam" -name '*.3dmap' 2>/dev/null | sort)
done

n=$(wc -l < "$OUT" | tr -d ' ')
echo "Wrote $OUT with $n job units."
