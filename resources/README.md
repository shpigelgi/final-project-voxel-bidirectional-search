# Resources

Reference material for the project: the papers, the external repos, and a map of
where the relevant code already lives inside HOG2.

## Papers (`papers/`)

All 7 PDFs from Lior's email are here (extracted from `fwd.zip`). Suggested reading
order (per the email: start with Siag; if clear, continue to Eckerle; otherwise read
in numbered order):

| # | File | Title |
|---|------|-------|
| 2 | `2-Siag-bidirectional-LB-theory.pdf` | Big-picture theory of the lower bound (LB) in bidirectional search |
| 3 | `3-Eckerle-2017-sufficient-conditions-node-expansion.pdf` | Sufficient Conditions for Node Expansion in Bidirectional Heuristic Search |
| 4 | `4-Shaham-2017-minimal-set-must-expand.pdf` | The Minimal Set of States that Must Be Expanded in a Front-to-End Bidirectional Search |
| 5 | `5-Shaham-2018-consistent-heuristics.pdf` | Minimizing Node Expansions in Bidirectional Search with Consistent Heuristics |
| 1 | `1-Holte-2017-MM-meet-in-the-middle.pdf` | MM: A bidirectional search algorithm guaranteed to meet in the middle |
| 6 | `6-Alcazar-2020-unifying-view-individual-bounds.pdf` | A Unifying View on Individual Bounds and Heuristic Inaccuracies in Bidirectional Search |
| — | `domain-Voxel-Benchmarks-3D-Pathfinding.pdf` | Voxel Benchmarks for 3D Pathfinding — Sandstone, Descent, and Industrial Plants |

## External repositories & links

| What | Link |
|------|------|
| Warthog voxel-map benchmarks | https://bitbucket.org/shortestpathlab/benchmarks/src/master/voxel-maps/ |
| HOG2 search framework | https://github.com/MovingAILab/hog2 |
| HOG2 usage example (SPL-BGU lab, BiHS) | https://github.com/SPL-BGU/BiHS-Direction-Choosing/tree/master/src/paper |
| LLM-search inference environment | https://github.com/sumedhpendurkar/Search-LLM-inference |

## Where the relevant code already lives in HOG2

A scan of the cloned HOG2 tree shows that **most of the building blocks already exist** —
the project is largely about *connecting* them to the Warthog benchmark, not writing
algorithms from scratch.

### Search algorithms — `hog2/generic/`
All the algorithms named in the brief are present:

| Brief name        | HOG2 file                              |
|-------------------|----------------------------------------|
| **A\***               | `TemplateAStar.h`                      |
| **MM** (meet-in-middle) | `MM.h`, `fMM.h`                       |
| **BAE\***             | `BAE.h`                                |
| **BiA\***             | bidirectional A\* (via `MM.h`/`fMM.h` config or `NBS.h` family) |
| **GBFS** (bidirectional) | `BidirectionalGreedyBestFirst.h`    |
| extras            | `NBS.h`, `DVCBS.h`, `AStarEpsilon.h`, `BidirectionalDijkstra.h` |

### Voxel domain — `hog2/environments/`
- `Voxels.h` / `Voxels.cpp` — the voxel world domain
- `VoxelGrid.h` / `VoxelGrid.cpp` — grid representation
- `Map3DGrid.h` / `Map3DGrid.cpp` — 3D grid

### Entry point — `hog2/apps/voxel/Sample.cpp`
Already includes `Voxels.h`, `VoxelGrid.h`, `TemplateAStar.h`, `NBS.h` and declares
`BuildBenchmarks()` / `SolveBenchmarks()`. This is the natural place to plug in the
Warthog benchmark loader and to loop over all the required algorithms.

> **Open question for the implementation:** confirm whether the Warthog `.3dmap` /
> voxel format matches what `Voxels.cpp` expects, or whether a small converter is
> needed. That converter (if any) belongs in `workspace/src/`.
