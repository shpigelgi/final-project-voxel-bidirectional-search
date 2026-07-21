#!/usr/bin/env bash
# Build the headless voxel experiment driver against the HOG2 clone.
# No OpenGL/GLUT needed: HOG2's search core is graphics-free; only the GUI
# driver (RunHOGGUI) pulls in GL, and we don't use it.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS="$(cd "$HERE/.." && pwd)"
HOG="$WS/hog2"
OUTDIR="${1:-$HERE}"
mkdir -p "$OUTDIR"

# Compiler is configurable so the BGU cluster (no system g++/clang; use a conda
# toolchain) can pass e.g. CXX=x86_64-conda-linux-gnu-g++ and
# XFLAGS="-static-libstdc++ -static-libgcc" for a portable, self-contained binary.
CXX="${CXX:-g++}"
XFLAGS="${XFLAGS:-}"

# Include every HOG2 source dir (space-safe: one dir, e.g. "gui/MAC/HID Support",
# contains a space and MUST be quoted — an unquoted flag list silently drops
# every -I and the build fails with "SearchEnvironment.h not found").
INCS=()
while IFS= read -r d; do INCS+=("-I$d"); done \
  < <(find "$HOG" -type d -not -path '*/build/*' -not -path '*/.git/*')

DEPS=("$HOG/utils/Timer.cpp" "$HOG/utils/FPUtil.cpp")

"$CXX" -std=c++17 -O2 $XFLAGS -o "$OUTDIR/voxdriver" "$HERE/driver.cpp" "${DEPS[@]}" "${INCS[@]}"
echo "Built: $OUTDIR/voxdriver"

"$CXX" -std=c++17 -O2 $XFLAGS -o "$OUTDIR/mvc" "$HERE/mvc.cpp" "${DEPS[@]}" "${INCS[@]}"
echo "Built: $OUTDIR/mvc"

"$CXX" -std=c++17 -O2 $XFLAGS -o "$OUTDIR/trace" "$HERE/trace.cpp" "${DEPS[@]}" "${INCS[@]}"
echo "Built: $OUTDIR/trace"

"$CXX" -std=c++17 -O2 $XFLAGS -o "$OUTDIR/validate" "$HERE/validate.cpp" "${DEPS[@]}" "${INCS[@]}"
echo "Built: $OUTDIR/validate"
