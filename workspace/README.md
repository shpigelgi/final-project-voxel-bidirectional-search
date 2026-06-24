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

## Building HOG2

HOG2 builds with `make` (no cmake needed) using the makefiles under `hog2/build/`.
On macOS you can also open the Xcode project under `hog2/build/XCode/`. To build the
voxel app from the command line, see HOG2's own `README.md` and the `hog2/build/`
makefiles. Typical flow:

```bash
cd hog2/build/gmake   # (path may vary by HOG2 version)
make voxel            # builds the apps/voxel target
```

## Requirements (already verified on this machine)

- `git` ✓
- C++ compiler (`clang`/`g++`) ✓
- OpenGL/GLUT may be needed for HOG2's GUI targets; headless benchmark runs avoid it.

See `../resources/README.md` for the map of which HOG2 files implement each algorithm
and the voxel domain.
