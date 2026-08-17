# Bidirectional Heuristic Search on 3D Voxel Maps

Code and data for the report of the same name (Gilad Shpigelman, Adam Rammal),
final project for **Search Methods in Artificial Intelligence**, Ben-Gurion
University of the Negev.

We run seven search algorithms on the Warthog 3D voxel benchmarks and measure
each one against a **per-instance lower bound on the expansions any correct
front-to-end algorithm must make** (the minimum vertex cover of the must-expand
graph), rather than against each other. This repository holds everything needed
to rebuild the binaries, re-run the sweep, and regenerate every number in the
report.

| | |
|---|---|
| Algorithms | A\*, reverse A\*, MM, NBS, BAE\*, BiA\* (ours), GBFS |
| Domains | industrial plants, sandstone, descent \([Nobes et al. 2023](https://bitbucket.org/shortestpathlab/benchmarks/src/master/voxel-maps/)\) |
| Movement | 6-connectivity and 26-connectivity, no corner cutting |
| Search core | [HOG2](https://github.com/MovingAILab/hog2), plus our voxel environment and BiA\* |

## Layout

```
src/          our code: voxel environment, BiA*, the experiment driver,
              the must-expand floor, the legality auditor, the trace writer
cluster/      the Slurm pipeline that produced the results, plus aggregation,
              plotting and verification scripts
scripts/      setup.sh (clone HOG2), fetch-benchmarks.sh (download the maps)
hog2-patches/ counters we add to HOG2's BAE* and NBS for the floor diagnostics
results/      the released per-instance data behind every reported number
```

`hog2/` and `benchmarks/` are fetched by the setup scripts and are not committed:
together they run to several GB.

## Setup

```bash
./scripts/setup.sh                      # clone HOG2 into hog2/ and apply our patch
./scripts/fetch-benchmarks.sh           # industrial-plants + sandstone
./scripts/fetch-benchmarks.sh all       # add descent (many GB) and warframe
./src/build.sh bin                      # build voxdriver, mvc, validate into bin/
```

Requirements: a C++17 compiler and Python 3 (standard library only). No GPU, no
Python packages. `build.sh` honours `CXX` and `XFLAGS`, which is how it is built
on a cluster with no system compiler:

```bash
CXX=x86_64-conda-linux-gnu-g++ XFLAGS="-static-libstdc++ -static-libgcc" ./src/build.sh bin
```

## Running a single instance

Map and scenario are positional; everything else is a flag.

```bash
M=benchmarks/voxel-maps/industrial-plants/plant01   # fetch-benchmarks.sh unzips here
./bin/voxdriver $M.3dmap $M.3dscen --algs astar,rastar,mm,nbs,bae,bia,gbfs \
                --limit 10 --timeout 60 --mem-mb 7000
./bin/mvc       $M.3dmap $M.3dscen --limit 10      # the must-expand floor
./bin/validate  $M.3dmap $M.3dscen                 # independent legality audit
```

Both binaries default to 26-connectivity; pass `--no-diagonals` for the
6-connected model. `--hweight w` scales the heuristic and is the knob behind the
heuristic-strength experiment (`w=1` full strength, `w=0` Dijkstra). `--start K`
and `--limit N` select a slice of the scenario file, which is how the Slurm array
shards the work.

Both write CSV to stdout, one row per (instance, algorithm), which is what
`cluster/aggregate.py` consumes.

## Reproducing the full sweep

The reported results come from a Slurm array on the BGU cluster.
`cluster/RUNBOOK.md` documents the allocation, the tunables and the gotchas.

```bash
cd cluster
./submit.sh                             # build, generate the manifest, submit the array
./submit.sh industrial-plants sandstone # or a subset of families
python3 aggregate.py <results-dir> ../results/cluster
```

Tunables are environment variables: `TIMEOUT` (per-instance seconds, default 60),
`MEMMB`, `ALGS`, `LIMIT`, `CONC`, `PARTITION`, `ACCOUNT`.

The heuristic-strength sweep is separate, and uses a 300 s cap because weakening
the heuristic grows the searches:

```bash
./run_crossover.sh                      # sweeps w over {1.0, 0.8, 0.6, 0.5, 0.4}
```

## Reproducing the reported numbers

Two scripts regenerate everything the report prints, from the data in `results/`.
Neither needs the cluster, the benchmarks or a build.

```bash
python3 results/verify_reported_figures.py   # every table and inline statistic
python3 cluster/figure_values.py             # every value plotted in a figure
```

`verify_reported_figures.py` recomputes 139 printed figures and reports any that
do not reproduce. `figure_values.py` recomputes the distributions, medians and
percentiles that the report's pgfplots figures are drawn from.

Three conventions matter if you recompute anything by hand, because a naive
calculation disagrees with the report on all three and the report is right:

- Ratios are formed **per instance and then medianed**, never as a ratio of two
  medians. The two differ substantially on heavy-tailed groups.
- Cross-algorithm comparisons are restricted to instances **every** algorithm in
  the comparison solved.
- `exp_over_mvc` is undefined when the floor is 0, which happens when the
  heuristic is exact from the start. Those instances are excluded from the floor
  axis but **must still count** for the direction and timing axes.

## What is in `results/`

| Path | Contents |
|---|---|
| `cluster/combined_long.csv` | one row per (instance, algorithm): expansions, cost, time, peak memory, status, floor, ratio |
| `cluster/summary.csv` | per (domain × movement mode × algorithm) aggregates |
| `subbound_instances.csv` | every instance scoring below the floor, for MM and NBS |
| `nbs_admissible_*.csv` | the `-DADMISSIBLE` rebuild comparison, both builds per instance |
| `nbs_batch2_mvc.csv` | floor, epsilon-refined floor and optimal split for those instances |
| `crossover/` | the heuristic-strength sweep |

## Two things worth knowing about HOG2

Both are documented in the report; they are repeated here because anyone reusing
this code will meet them.

- **BAE\*** rounds its lower bound to the greatest common divisor of the edge
  costs, defaulting to 1. Under 26-connectivity the costs are
  `{1, √2, √3}` and share no common measure, so the rounding is invalid and the
  search returns suboptimal paths silently. We set the gcd to `1e-6`.
- **NBS** discards a generated node already closed on the opposite frontier
  unless `ADMISSIBLE` is defined, which `BDOpenClosed.h` leaves commented out.
  That rule is sound only under a consistent heuristic and it takes NBS outside
  the class the must-expand bound covers. Build with `XFLAGS=-DADMISSIBLE` for
  the in-class behaviour.
