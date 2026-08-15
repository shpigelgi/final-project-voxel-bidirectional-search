#!/usr/bin/env bash
# Representative "run-until-stable" sweep — the exact protocol behind the paper's
# §Sampling and convergence. Captures what the committed make_manifest.sh does NOT:
# instance shuffling + chunked, mode-split, shuffled-scenario manifests.
#
# Why this exists: the benchmark scenario files are difficulty-graded, so a
# contiguous first-N prefix is a biased (easy) sample. We shuffle each map's
# instances with a fixed seed so any prefix is representative, then sweep with a
# bootstrap-median convergence stopping rule (see convergence.py).
#
#   ./run_convergence.sh [SEED] [PER_MAP] [CHUNK]
# Defaults: SEED=20260802 PER_MAP=500 CHUNK=100  (round 1; extend toward 2000 as needed)
set -euo pipefail
CLUSTER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; WS="$(dirname "$CLUSTER_DIR")"
SEED="${1:-20260802}"; PER_MAP="${2:-500}"; CHUNK="${3:-100}"
VOX="$WS/benchmarks/voxel-maps"

# 1. Seeded per-map shuffle -> *.shuf.3dscen (+ SHUFFLE_SEED.txt). Idempotent.
python3 "$CLUSTER_DIR/shuffle_scens.py" "$SEED" "$VOX"

# 2. Chunked manifests over the SHUFFLED scenarios, one per mode (memory differs).
#    Every map is chunked uniformly and all run in parallel, so a family sample
#    accumulates balanced across maps (not dominated by the first maps).
for mode in diag nodiag; do
  MAN="$CLUSTER_DIR/manifest.conv.$mode.tsv"; : > "$MAN"
  for mapf in $(find "$VOX" -name '*.3dmap' | sort); do
    base="$(basename "$mapf" .3dmap)"
    fam="$(echo "$mapf" | sed -E 's#.*/voxel-maps/([^/]+)/.*#\1#')"
    scen="$(find "$VOX" -name "$base.shuf.3dscen" | head -1)"; [ -z "$scen" ] && continue
    s=0; while [ "$s" -lt "$PER_MAP" ]; do
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$fam" "$mapf" "$scen" "$mode" "$s" "$CHUNK" >> "$MAN"
      s=$((s+CHUNK))
    done
  done
  echo "$mode manifest: $(wc -l < "$MAN" | tr -d ' ') tasks"
done

# 3. Submit, mode-split memory (nodiag is light; diag is where MM's footprint blows up).
export ROOT="$WS" TIMEOUT=60 ALGS=astar,rastar,mm,bia,bae,nbs,gbfs LIMIT="$CHUNK"
# NOTE on the memory caps: MEMMB is an RLIMIT_AS (virtual) cap. An initial nodiag
# cap of 1800 MB was sized from an easy-sample RSS and turned out too tight on the
# representative sample (bad_alloc -> SIGABRT, mislabeled 'error'; ~767 hits, mostly
# descent-nodiag), while diag at 7000 had none. Sized up to 4000 MB from the
# representative peaks. RSS understates need because RLIMIT_AS bounds virtual, not RSS.
export MANIFEST="$CLUSTER_DIR/manifest.conv.nodiag.tsv" MEMMB=4000
NN=$(wc -l < "$MANIFEST" | tr -d ' ')
sbatch --parsable --array="1-${NN}%150" --export=ALL --partition=cpu --mem=5G --time=12:00:00 \
  --job-name=vox-conv-nd "$CLUSTER_DIR/run_array.sbatch"
export MANIFEST="$CLUSTER_DIR/manifest.conv.diag.tsv" MEMMB=7000
ND=$(wc -l < "$MANIFEST" | tr -d ' ')
sbatch --parsable --array="1-${ND}%150" --export=ALL --partition=cpu --mem=8G --time=12:00:00 \
  --job-name=vox-conv-dg "$CLUSTER_DIR/run_array.sbatch"

# 4. After completion: concat_chunks.sh per family, aggregate.py, then convergence.py
#    to see which (family,mode,alg) groups hit +/-3% CI and which to extend.
echo "submitted. when done: concat_chunks -> aggregate.py -> convergence.py"
