#!/usr/bin/env bash
# Build the headless voxel experiment driver against the HOG2 clone.
# No OpenGL/GLUT needed: HOG2's search core is graphics-free; only the GUI
# driver (RunHOGGUI) pulls in GL, and we don't use it.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS="$(cd "$HERE/.." && pwd)"
HOG="$WS/hog2"
OUT="${1:-$HERE/voxdriver}"

# Include every HOG2 source dir (space-safe: one dir, e.g. "gui/MAC/HID Support",
# contains a space and MUST be quoted — an unquoted flag list silently drops
# every -I and the build fails with "SearchEnvironment.h not found").
INCS=()
while IFS= read -r d; do INCS+=("-I$d"); done \
  < <(find "$HOG" -type d -not -path '*/build/*' -not -path '*/.git/*')

g++ -std=c++17 -O2 -o "$OUT" \
    "$HERE/driver.cpp" \
    "$HOG/utils/Timer.cpp" \
    "$HOG/utils/FPUtil.cpp" \
    "${INCS[@]}"

echo "Built: $OUT"
