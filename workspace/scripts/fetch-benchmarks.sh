#!/usr/bin/env bash
# Download ONLY the voxel-maps subdirectory of the Warthog/shortestpathlab
# benchmarks monorepo, using a sparse + shallow checkout (the full repo is huge).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(dirname "$SCRIPT_DIR")"
BENCH_DIR="$WORKSPACE_DIR/benchmarks"
REPO="https://bitbucket.org/shortestpathlab/benchmarks.git"
SUBDIR="voxel-maps"

if [ -d "$BENCH_DIR/.git" ]; then
  echo "Benchmarks repo already present at $BENCH_DIR — pulling latest."
  git -C "$BENCH_DIR" pull --ff-only
  exit 0
fi

echo "Sparse-cloning '$SUBDIR' from $REPO ..."
git clone --filter=blob:none --no-checkout --depth 1 "$REPO" "$BENCH_DIR"
git -C "$BENCH_DIR" sparse-checkout init --cone
git -C "$BENCH_DIR" sparse-checkout set "$SUBDIR"
git -C "$BENCH_DIR" checkout

echo
echo "Done. Voxel maps are at: $BENCH_DIR/$SUBDIR"
du -sh "$BENCH_DIR/$SUBDIR" 2>/dev/null || true
