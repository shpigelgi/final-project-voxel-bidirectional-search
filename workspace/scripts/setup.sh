#!/usr/bin/env bash
# Clone the HOG2 search framework into workspace/hog2.
# HOG2 is gitignored (it's 60+ MB and an external dependency), so each
# clone of THIS repo needs to run this once.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(dirname "$SCRIPT_DIR")"
HOG2_DIR="$WORKSPACE_DIR/hog2"

if [ -d "$HOG2_DIR/.git" ]; then
  echo "HOG2 already present at $HOG2_DIR — pulling latest."
  git -C "$HOG2_DIR" pull --ff-only
else
  echo "Cloning HOG2 into $HOG2_DIR ..."
  git clone https://github.com/MovingAILab/hog2.git "$HOG2_DIR"
fi

echo
echo "Done. HOG2 is at: $HOG2_DIR"
echo "Next: run scripts/fetch-benchmarks.sh to download the Warthog voxel maps."
