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

# Sources are grouped by role: core/ (shared domain + algorithm headers, no .cpp),
# experiment/ (the measurement binaries), viz/ (the visualizer trace generator).
# Each .cpp reaches the headers via an explicit "../core/..." include, so no extra
# -I is needed for our own code.
build() {  # build <output-name> <source-path>
	"$CXX" -std=c++17 -O2 $XFLAGS -o "$OUTDIR/$1" "$HERE/$2" "${DEPS[@]}" "${INCS[@]}"
	echo "Built: $OUTDIR/$1"
}

build voxdriver experiment/driver.cpp
build mvc       experiment/mvc.cpp
build validate  experiment/validate.cpp
build trace     viz/trace.cpp
