# `src/` — our code

Code **we** write lives here, kept separate from the upstream HOG2 clone (`../hog2/`)
so it's easy to see what is ours and to keep it under version control.

Likely contents as the project progresses:

- **Warthog → HOG2 voxel loader / converter** — read the Warthog voxel-map format and
  feed it into HOG2's `Voxels` domain (if the formats don't already match).
- **Experiment driver** — a routine (or a modified copy of `hog2/apps/voxel/Sample.cpp`)
  that loops over every required algorithm (A\*, MM, BAE\*, BiA\*, GBFS + unidirectional),
  runs each from both directions, with and without diagonal movement, and records
  metrics (nodes expanded, runtime, solution cost, optimality).
- **Result aggregation** — scripts that turn raw runs into the tables/plots in
  `../../deliverables/`.

## How this links into HOG2

HOG2 builds via the makefiles under `hog2/build/`. Two common approaches:

1. **Edit in place:** modify `hog2/apps/voxel/Sample.cpp` directly and rebuild that app.
   Simplest, but mixes our changes into the upstream clone (which is gitignored).
2. **Keep ours here:** put new `.cpp/.h` in this folder and point the build at them.
   Cleaner provenance; this is preferred once changes grow beyond a few lines.

Document whichever you choose in `../README.md`.
