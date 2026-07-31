# Workspace

The working area where the code is built and experiments are run.

```
workspace/
├── hog2/         # HOG2 search framework (cloned; gitignored — see scripts/setup.sh)
├── benchmarks/   # Warthog voxel maps (downloaded; gitignored — see scripts/fetch-benchmarks.sh)
├── src/          # OUR code: loaders, experiment driver, aggregation (see src/README.md)
└── scripts/      # setup & helper scripts
```

`hog2/` and `benchmarks/` are **not committed** — they're large external dependencies.
Anyone cloning this repo recreates them with the scripts below.

## First-time setup

```bash
cd workspace
./scripts/setup.sh             # clone HOG2 into hog2/
./scripts/fetch-benchmarks.sh  # sparse-download the Warthog voxel maps into benchmarks/
```

## Building (headless — no OpenGL/GLUT)

We do **not** use HOG2's GUI app. Our tools link only HOG2's graphics-free search core:

```bash
./src/build.sh workspace/bin      # -> bin/{voxdriver,mvc,validate,trace}
```

- `bin/voxdriver` — runs the algorithms on a map/scenario, CSV out, per-instance timeout
  and memory cap (fork-per-run). See `src/experiment/driver.cpp`.
- `bin/mvc` — per-instance must-expand floor (Minimum Vertex Cover). See `src/experiment/mvc.cpp`.
- `bin/validate` — independent legality auditor. See `src/experiment/validate.cpp`.
- `bin/trace` — JSON search trace for the 3D visualizer. See `src/viz/trace.cpp`.

Sources are grouped by role (`src/core/` shared, `src/experiment/`, `src/viz/`); see
[`src/README.md`](src/README.md).

Only needs `git` and a C++17 compiler (`g++`/`clang++`). No cmake, no OpenGL.

## Running experiments

- **Locally / quick:** `bin/voxdriver <map.3dmap> <scen.3dscen> --limit N` (see `--help` args
  in `src/experiment/driver.cpp`); `bin/mvc <map> <scen>` for the floor.
- **On the cluster:** see [`cluster/RUNBOOK.md`](cluster/RUNBOOK.md) — `submit.sh` builds,
  generates a manifest, and submits a Slurm array (one task per map×config); `aggregate.py`
  joins results with the MVC floor into summary tables.

See `src/FINDINGS.md` for the format spec, HOG2 map, and results so far, and
`../resources/README.md` for which HOG2 files implement each algorithm and the voxel domain.
