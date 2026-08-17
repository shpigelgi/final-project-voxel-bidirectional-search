#!/usr/bin/env bash
# Clone the HOG2 search framework into hog2/, at the exact revision the paper used.
# HOG2 is gitignored (it's 60+ MB and an external dependency), so each
# clone of THIS repo needs to run this once.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(dirname "$SCRIPT_DIR")"
HOG2_DIR="$WORKSPACE_DIR/hog2"

# The revision every number in the report was produced with. Upstream's default
# branch is PDB-refactor and it moves, so this is pinned: an unpinned clone would
# quietly hand a later HOG2 to anyone reproducing the results, where our patches
# may fail to apply or, worse, apply to changed code. Bump it only together with
# a re-run of the sweep.
HOG2_REV="af9d42d06827e5bd96d093c17e4d98b5e4f99e79"
HOG2_URL="https://github.com/MovingAILab/hog2.git"

if [ -d "$HOG2_DIR/.git" ]; then
  echo "HOG2 already present at $HOG2_DIR."
else
  echo "Cloning HOG2 into $HOG2_DIR ..."
  git clone "$HOG2_URL" "$HOG2_DIR"
fi

# A shallow or stale clone will not have the pinned commit. Fetch it if missing,
# then detach onto it. Never build whatever HEAD happens to be.
if ! git -C "$HOG2_DIR" cat-file -e "${HOG2_REV}^{commit}" 2>/dev/null; then
  echo "Fetching pinned revision ..."
  git -C "$HOG2_DIR" fetch origin "$HOG2_REV" 2>/dev/null \
    || git -C "$HOG2_DIR" fetch origin
fi
git -C "$HOG2_DIR" checkout --detach "$HOG2_REV"
echo "HOG2 pinned at $HOG2_REV"

echo
echo "Done. HOG2 is at: $HOG2_DIR"
echo "Next: run scripts/fetch-benchmarks.sh to download the Warthog voxel maps."
# HOG2 is gitignored, so these counters live as patch scripts, applied here. No output
# silencing: a patch that fails to apply must abort setup (set -e) rather than look like a
# success, which is how a missing patch previously went unnoticed. Both are idempotent.
python3 "$(dirname "$0")/../hog2-patches/add_bae_nipped.py"      # nodesNipped counter for BAE* (floor diagnostic)
python3 "$(dirname "$0")/../hog2-patches/add_nbs_discarded.py"   # nodesDiscarded counter for NBS (sub-floor diagnostic)
