# Bidirectional Heuristic Search on Voxel Maps

Final project — **Search Methods in Artificial Intelligence**, Ben-Gurion University.
Advisor: **Lior Siag**.

## Goal

Run **bidirectional search algorithms on 3D Voxel maps** and analyze them. Concretely:
**connect the Warthog voxel-map benchmark to the bidirectional-search code in HOG2**,
then run every required algorithm from both directions.

- **Algorithms:** `A*`, `MM`, `BAE*`, `BiA*`, `GBFS`, plus unidirectional variants — run **from both sides**.
- **Movement:** **without** diagonal movement; **also with** diagonals *if* the domain already supports it.
- **Maps:** the Warthog `voxel-maps` benchmark (sandstone, descent, industrial-plant scenes).

The full brief (original Hebrew + English translation, with all links) is in
[`instructions/email-from-lior.md`](instructions/email-from-lior.md).

## Repository layout

```
hw-01/
├── README.md            ← you are here
├── instructions/        The assignment as given
│   └── email-from-lior.md
├── resources/           Reference material (read-only inputs)
│   ├── README.md            annotated bibliography + map of where code lives in HOG2
│   └── papers/              the 7 PDFs (Siag, Eckerle, Shaham, Holte, Alcazar, domain paper)
├── workspace/           Where work happens (build + run)
│   ├── hog2/                HOG2 framework  — cloned, gitignored
│   ├── benchmarks/          Warthog voxel maps — downloaded, gitignored
│   ├── src/                 OUR code (loaders, experiment driver, aggregation)
│   └── scripts/             setup.sh, fetch-benchmarks.sh
└── deliverables/        What gets handed in
    ├── report/              the write-up
    └── results/             final tables & plots
```

**Why this split:** `instructions/` and `resources/` are fixed inputs; `workspace/` is
the messy build/run area where large external deps (HOG2, benchmarks) live but are
**not committed**; `deliverables/` holds only the curated final output. This keeps the
git repo small and makes it obvious what is *ours* versus upstream.

## Getting started

```bash
# 1. Pull the external dependencies (they are gitignored)
cd workspace
./scripts/setup.sh             # clones HOG2 into workspace/hog2/
./scripts/fetch-benchmarks.sh  # sparse-downloads Warthog voxel maps into workspace/benchmarks/

# 2. Add the papers
#    Drop the 7 PDFs from Lior's email into resources/papers/
```

Then build the HOG2 voxel app — see [`workspace/README.md`](workspace/README.md).

## What already exists (good news)

A scan of HOG2 shows the heavy lifting is largely *present*:

- **Algorithms** live in `hog2/generic/`: `TemplateAStar.h` (A\*), `MM.h`/`fMM.h`,
  `BAE.h`, `BidirectionalGreedyBestFirst.h` (GBFS), plus `NBS.h`, `DVCBS.h`.
- **Voxel domain** lives in `hog2/environments/`: `Voxels.{h,cpp}`, `VoxelGrid.{h,cpp}`.
- **An entry point** exists at `hog2/apps/voxel/Sample.cpp` with `BuildBenchmarks()` /
  `SolveBenchmarks()` already wired to the voxel domain and to A\*/NBS.

So the core work is: **(a)** make the Warthog voxel format loadable by HOG2's `Voxels`
domain (a small converter may be needed), and **(b)** extend the driver to run *all*
required algorithms from both sides and record metrics. Details and the file-by-file
map are in [`resources/README.md`](resources/README.md).

## Key references

| | |
|---|---|
| HOG2 | https://github.com/MovingAILab/hog2 |
| HOG2 usage example (SPL-BGU) | https://github.com/SPL-BGU/BiHS-Direction-Choosing/tree/master/src/paper |
| Warthog voxel benchmarks | https://bitbucket.org/shortestpathlab/benchmarks/src/master/voxel-maps/ |
| LLM-search inference env | https://github.com/sumedhpendurkar/Search-LLM-inference |

## Status

- [x] Workspace scaffolded; HOG2 cloned locally; documentation written.
- [ ] Papers added to `resources/papers/`.
- [ ] Benchmarks downloaded.
- [ ] Warthog → HOG2 voxel loader confirmed/written.
- [ ] All algorithms run from both sides; results collected.
- [ ] Report written.
