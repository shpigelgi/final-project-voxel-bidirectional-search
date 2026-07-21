#!/usr/bin/env bash
# Download and unzip Warthog voxel-map benchmarks for selected families.
#
#   ./fetch-benchmarks.sh                     # default: industrial-plants sandstone
#   ./fetch-benchmarks.sh industrial-plants   # one family
#   ./fetch-benchmarks.sh all                 # all four (descent is HUGE: many GB)
#
# Uses a shallow, sparse git checkout of only the requested families, then
# unzips every .3dmap.zip in place (the driver/mvc read the plain .3dmap text).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(dirname "$SCRIPT_DIR")"
BENCH_DIR="$WORKSPACE_DIR/benchmarks"
REPO="https://bitbucket.org/shortestpathlab/benchmarks.git"

FAMILIES=("$@")
if [ ${#FAMILIES[@]} -eq 0 ]; then FAMILIES=(industrial-plants sandstone); fi
if [ "${FAMILIES[0]}" = "all" ]; then FAMILIES=(industrial-plants sandstone descent warframe); fi

echo "Fetching families: ${FAMILIES[*]}"

if [ ! -d "$BENCH_DIR/.git" ]; then
  echo "Sparse-cloning benchmarks repo into $BENCH_DIR ..."
  git clone --filter=blob:none --no-checkout --depth 1 "$REPO" "$BENCH_DIR"
  git -C "$BENCH_DIR" sparse-checkout init --cone
fi

PATHS=()
for fam in "${FAMILIES[@]}"; do PATHS+=("voxel-maps/$fam"); done
git -C "$BENCH_DIR" sparse-checkout set "${PATHS[@]}"
git -C "$BENCH_DIR" checkout

echo "Unzipping map files ..."
count=0
while IFS= read -r z; do
  unzip -o -q "$z" -d "$(dirname "$z")" && count=$((count+1))
done < <(find "$BENCH_DIR/voxel-maps" -name '*.3dmap.zip')
echo "Unzipped $count map archives."

echo
echo "Done. Benchmarks under: $BENCH_DIR/voxel-maps"
for fam in "${FAMILIES[@]}"; do
  n_map=$(find "$BENCH_DIR/voxel-maps/$fam" -name '*.3dmap' 2>/dev/null | wc -l | tr -d ' ')
  n_scn=$(find "$BENCH_DIR/voxel-maps/$fam" -name '*.3dscen' 2>/dev/null | wc -l | tr -d ' ')
  echo "  $fam: $n_map maps, $n_scn scenarios"
done
