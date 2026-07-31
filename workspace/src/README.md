# `src/` — our code

Code **we** wrote, kept separate from the upstream HOG2 clone (`../hog2/`) so it's easy
to see what is ours. ~1,100 lines. Build everything with `./build.sh <outdir>`.

```
src/
├── core/           # shared: the domain and the one algorithm HOG2 lacks
│   ├── VoxelMap.h  #   headless voxel SearchEnvironment + .3dmap loader
│   └── BiAStar.h   #   Pohl-style bidirectional A* (BS*)
├── experiment/     # produces the numbers in deliverables/results/
│   ├── driver.cpp  #   → bin/voxdriver : run the algorithms, emit CSV
│   ├── mvc.cpp     #   → bin/mvc       : per-instance must-expand floor
│   └── validate.cpp#   → bin/validate  : independent legality auditor
├── viz/            # feeds the interactive UI
│   └── trace.cpp   #   → bin/trace     : JSON trace for search-visualizer.html
├── build.sh        # one command, all four binaries
└── FINDINGS.md     # the engineering narrative (formats, HOG2 quirks, results)
```

## The split

**`core/`** is header-only and shared by both sides. `VoxelMap.h` implements HOG2's
`SearchEnvironment<voxState,voxAction>`, so every upstream algorithm (A\*, MM, BAE\*,
NBS, GBFS) runs on Warthog voxel maps with no GUI; because `SearchEnvironment` inherits
`Heuristic`, the env doubles as the forward/backward heuristic. `BiAStar.h` is the
brief's "BiA\*", which has no HOG2 class.

**`experiment/`** is the measurement path — everything whose output ends up in a CSV,
a table, or a figure. Read `driver.cpp` for how a run is executed (fork-per-run with a
wall-clock timeout and an `RLIMIT_AS` cap, so one pathological instance can't take down
a Slurm task), and `mvc.cpp` for the denominator that makes expansions comparable
across instances.

**`viz/`** is the presentation path — only `trace.cpp`, which dumps one search as JSON
for `../../deliverables/search-visualizer.html` (the page itself is a handed-in
artifact, so it lives in `deliverables/`). Nothing in `experiment/` depends on `viz/`
or vice versa; they meet only at `core/`.

Note that figure generation is *not* here: `../cluster/plot_results.py` turns the
aggregated cluster CSVs into the report PDFs, and belongs to the cluster pipeline
(`submit.sh` → `run_array.sbatch` → `aggregate.py` → `plot_results.py`).

## Building

```bash
./build.sh ../bin      # -> ../bin/{voxdriver,mvc,validate,trace}
```

`build.sh` honours `$CXX` and `$XFLAGS` so the BGU cluster (no system g++/clang) can
pass a conda toolchain and static libstdc++; see `../cluster/RUNBOOK.md`. Each `.cpp`
includes its headers as `"../core/..."`, so no extra `-I` is needed for our own code —
the `-I` list in `build.sh` is purely for HOG2.
