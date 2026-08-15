# Cluster Runbook — Bidirectional Voxel Search Experiments

How to run the full experiment set on the BGU Slurm cluster. Everything is headless
(no OpenGL/GLUT). All commands are run from `workspace/`.

## 0. One-time: adjust for the BGU allocation
Edit `cluster/run_array.sbatch` (or export at submit time):
- Uncomment/adjust `module load gcc` for the cluster toolchain (needs a C++17 g++).
- Set the partition/account either by uncommenting the `#SBATCH` lines or by exporting
  `PARTITION=...` / `ACCOUNT=...` before `submit.sh` (it forwards them to `sbatch`).
- Match `#SBATCH --mem` to `MEMMB` (keep `MEMMB` a bit below the SBATCH mem).

## 1. Get the code and dependencies
```bash
cd workspace
./scripts/setup.sh                        # clone HOG2 into workspace/hog2/
./scripts/fetch-benchmarks.sh             # default: industrial-plants + sandstone
#   ./scripts/fetch-benchmarks.sh all     # add descent (HUGE, many GB) + warframe
```

## 2. Build (headless)
```bash
./src/build.sh workspace/bin              # -> bin/voxdriver, bin/mvc
```
`submit.sh` also builds automatically; this step just lets you verify it compiles.

## 3. Submit the Slurm array
```bash
# defaults: TIMEOUT=60s/instance, MEMMB=9000, all 6 algorithms, all instances, 20 concurrent
./cluster/submit.sh                       # every family present under benchmarks/
./cluster/submit.sh industrial-plants     # or restrict to families

# tunables (env vars):
TIMEOUT=120 MEMMB=16000 CONC=40 ./cluster/submit.sh sandstone
LIMIT=200 ./cluster/submit.sh             # cap instances/map for a quick pass
```
- One array task = one (map, config) unit; config ∈ {diag (26-conn), nodiag (6-conn)}.
- Each `(instance, algorithm)` runs in a forked child with a wall-clock **timeout** and an
  RLIMIT_AS **memory cap**, so a single hard instance cannot hang or OOM the node.
- Rows are flushed as they complete, so a task killed by the SBATCH time limit still leaves
  partial results on disk.

Watch progress: `squeue -u $USER`; logs in `cluster/logs/%A_%a.out`.

## 4. Aggregate
```bash
./cluster/aggregate.py workspace/results
```
Writes `workspace/results/aggregated/`:
- `combined_long.csv` — one row per (family, map, config, instance, algorithm) with the
  expansion count joined to the instance's must-expand floor (`exp_over_mvc`).
- `summary.csv` — per (family, config, algorithm): `%ok`, `%timeout`, `%fail`,
  mean expanded, mean/median expanded-over-MVC, mean time.
- Console: an **optimality-consistency** check (the optimal algorithms must agree on cost
  per instance — the only optimality check in nodiag mode, see note below).

Copy the curated `summary.csv` (and any plots) into `deliverables/results/` for the report.

## Notes / gotchas
- **Diagonal vs. no-diagonal:** the scenario `optimal` cost column is computed with diagonals
  (26-connected). In `diag` runs the driver validates each solve against it (`status=ok`).
  In `nodiag` runs that cost does **not** apply, so optimality is instead verified by
  cross-algorithm agreement in `aggregate.py`. `status=ok` in nodiag means "completed".
- **Descent memory:** the largest descent maps are ~1.3 B voxels (~160 MB occupancy bitset)
  plus search memory; give those tasks generous `--mem`. Start with plants/sandstone.
- **`mvc` cost:** the must-expand floor expands each direction's full `f<C*` contour, which
  can be heavier than a single search on hard instances; it shares the same memory budget.
- **Metrics currency:** node expansions (report as ratio to the MVC floor); plus runtime,
  path cost, and solution status. GBFS is not cost-optimal — reported separately (`subopt`).
