# Repository Guide — start here

Everything needed to read the project and see the results is committed — **you do not
need to run anything.** (The two large external dependencies, HOG2 and the benchmark
maps, are intentionally *not* committed — they're multi-GB and regenerable; see the note
at the bottom. All of *our* code, and all *results/figures*, are here.)

## Read the results without running anything
- **`deliverables/results/cluster/RESULTS.md`** — the write-up: medians table, win-rate,
  directional asymmetry, GBFS trade-off, and the heuristic-strength **crossover**.
- **`deliverables/results/cluster/summary.csv`** — per (domain × movement-mode × algorithm):
  %ok, %timeout, mean/median expansions-over-floor, mean time.
- **`deliverables/results/cluster/combined_long.csv`** — the full per-instance raw data.
- **`deliverables/results/crossover/`** — the weak-heuristic sweep data.
- **`deliverables/report/figures/*.pdf`** (and `.png`) — the four figures:
  `exp_over_mvc`, `winrate`, `directional_asymmetry`, `crossover`.

## The code we wrote (`workspace/src/`, ~1,100 lines)
Read in this order:
1. **`VoxelMap.h`** — headless voxel environment + loader (`voxel`/`rev_voxel`; 26- and
   6-connectivity; octile costs; no-corner-cutting; the `--hweight` knob).
2. **`BiAStar.h`** — Pohl-style bidirectional A\* (the brief's "BiA\*", absent from HOG2).
3. **`driver.cpp`** — experiment runner (both directions; fork + timeout + memory cap; CSV).
4. **`mvc.cpp`** — the theoretical floor (Minimum Vertex Cover of the must-expand graph).
5. **`validate.cpp`** — independent legality auditor (no illegal / clipping moves).
6. **`trace.cpp`** — search trace for the 3D visualizer.

`workspace/src/FINDINGS.md` is the engineering narrative tying these together, incl. the
HOG2 quirks we hit and the BAE\* bug we fixed.

## How the results were produced (`workspace/cluster/`)
Slurm pipeline for BGU HPC: `submit.sh` → `run_array.sbatch` → `aggregate.py` →
`plot_results.py`; `crossover.sbatch` for the weak-heuristic study.
`cluster/RUNBOOK.md` + `.claude/skills/bgu-cluster/SKILL.md` document how to re-run it.

## Report material
- `deliverables/report/REPORT_GUIDE.md` — section-by-section scaffold for writing the report.
- `deliverables/report/background-and-plan.pdf` — the theory/background synthesis (source
  papers in `resources/papers/`).
- `deliverables/search-visualizer.html` — interactive 3D replay of the search.

## Not committed (external, regenerate only if you want to *build/run*)
- `workspace/hog2/` — the HOG2 framework (clone via `workspace/scripts/setup.sh`).
- `workspace/benchmarks/` — the Warthog voxel maps (download via `workspace/scripts/fetch-benchmarks.sh`).
- Build outputs (`workspace/bin/`), raw per-map cluster dumps, logs.
These are only needed to *rebuild and re-run*; reading the code and results needs none of them.
