# Engineering Findings — Warthog Voxel Benchmarks × HOG2

Status as of the initial spike. This resolves the README's "open question" about
the map format and de-risks the build. **Bottom line: the hard domain parts already
exist in HOG2 and a headless build works; the remaining work is a thin loader +
experiment driver, both of which now exist in `workspace/src/`.**

---

## 1. File formats (confirmed against real files + repo READMEs)

### Map — `.3dmap` (shipped as `.3dmap.zip`)
```
<map-type> <width> <height> <depth>      # map-type ∈ { voxel, rev_voxel }
x1 y1 z1
x2 y2 z2
...
```
- `voxel` → listed coords are **filled/obstacles**; `rev_voxel` → listed coords are
  **free** (everything else blocked). Dense maps (sandstone) use `rev_voxel`; industrial
  plants use `voxel` (verified: `plant01.3dmap` = `voxel 195 128 100`, 87 620 obstacles).
- Coordinate indexing starts at **0**. Total voxels = w·h·d.

### Scenario — `.3dscen`
```
version 2
plant01.3dmap
sx sy sz  tx ty tz  cost  h-error  [d-bias]
18 79 42  18 78 43  1.41421  1.0  1.0
```
- 6 coords, then **optimal cost**, heuristic error, and (v2) a directional-bias ratio.
- 2000 instances per map. The optimal cost is our ground-truth for validating solvers.
- ⚠️ HOG2's stock `SolveBenchmarks` expects `version 1` + 8 fields and will **not** parse
  these `version 2` / 9-field files. Our driver parses them correctly.

---

## 2. HOG2 reality check

| Component | Verdict |
|---|---|
| `environments/VoxelGrid.{h,cpp}` | ✅ The real search env. 26-connectivity, **no-corner-cutting** (`CanMove`), octile-3D costs `{1,√2,√3}`, consistent 3D-octile `HCost`. Reads the `voxel` text format. |
| `environments/Voxels.{h,cpp}` | ⚠️ **Stub for search** — `GetSuccessors`/`ApplyAction` empty, `HCost`=0, `GCost`=1. Reads the *binary* Morton `.3dnav`; useful only for its `-convert` (binary→text). Ignore for search. |
| Loader gaps | `VoxelGrid` handles `voxel` only (no `rev_voxel`), and doesn't unzip. |
| `apps/voxel/Sample.cpp` | GUI app (`RunHOGGUI`); its `-solve` path is stale (see above) and only runs A*/NBS, capped at 1000. |

### The build unlock
HOG2's search core is **graphics-free**: `utils/Graphics.h` is a device-independent
display-list abstraction with **no** OpenGL includes, and there are zero GL/GLUT includes
anywhere in `generic/ search/ utils/ graphics/ shared/`. GL/GLUT live only in the GUI
driver, which we don't use. So a headless build needs only two `.cpp` files
(`utils/Timer.cpp`, `utils/FPUtil.cpp`) plus the header-only algorithm templates.

> Gotcha: one HOG2 dir is named `gui/MAC/HID Support` (contains a space). Building the
> `-I` list unquoted silently drops **all** include dirs → "SearchEnvironment.h not found".
> `build.sh` uses a quoted bash array to avoid this.

---

## 3. Our code (`workspace/src/`)

- **`VoxelMap.h`** — headless `SearchEnvironment<voxState,voxAction>`. Loads `voxel`
  **and** `rev_voxel`. Move model / costs / heuristic copied verbatim from `VoxelGrid`,
  plus an `allowDiagonals` flag (the brief's "without diagonal movement" = 6 face moves,
  Manhattan heuristic). Because `SearchEnvironment : Heuristic`, the env doubles as the
  fwd/bwd heuristic for bidirectional algorithms.
- **`driver.cpp`** — headless experiment driver. Parses `version 2` scenarios, runs
  A\*, reverse A\*, MM, BAE\*, NBS, GBFS from both directions, emits CSV
  (`instance,alg,expanded,generated,cost,optimal,time_ms,optimal_ok`).
- **`mvc.cpp`** — computes the per-instance **must-expand floor** = Minimum Vertex Cover
  of G_MX (Eckerle/Chen; Shaham 2017 threshold scan, base admissible case). Expands each
  direction's f<C\* contour, buckets g-values, and minimizes `#{g_F<τ}+#{g_B<C*−τ}`.
  Emits `instance,cstar,fwd_cand,bwd_cand,mvc`. This is the denominator for the
  expansion-ratio analysis the papers use.
- **`build.sh`** — one-command headless build (`./build.sh` → `src/voxdriver` + `src/mvc`).

Run: `./voxdriver <map.3dmap> <scen.3dscen> [--limit N] [--no-diagonals] [--algs ...]`
(unzip the `.3dmap.zip` first, or add zip handling — currently done in the fetch step).

---

## 4. Validation (plant01, ~72-instance preview — pipeline check, not a result)

Costs verified against the scenario's optimal column:

| alg | avg expanded | optimal? | note |
|---|---:|---|---|
| astar | ~2 530 | ✅ all | fewest expansions here |
| nbs | ~8 980 | ✅ all | |
| bae | ~16 140 | ⚠️ **5 suboptimal** | see below |
| mm | ~24 470 | ✅ all | avg ~327 ms — **very slow** per instance |
| rastar | ~33 240 | ✅ all | strong directional bias vs astar |
| gbfs | ~200 | n/a | not cost-optimal by design |

Two real findings from the spike:

1. **BAE\* was returning slightly suboptimal paths on irrational (√2/√3) edge costs —
   now RESOLVED.** Root cause: `getLowerBound()` (BAE.h:179) rounds the `b`-bound *up* to
   the next multiple of `gcd`, and the constructor defaults `gcd = 1.0`. That round-up is
   only valid when every solution cost is a multiple of `gcd`; voxel costs `{1,√2,√3}`
   share no common divisor, so the bound was inflated to the next integer and the
   termination test `currentCost <= getLowerBound()` fired early. Fix (driver-side, no
   HOG2 edit): construct `BAE(true, /*epsilon*/1.0, /*gcd*/1e-6)` so the rounding is
   negligible. After the fix BAE\* matches the optimal cost on all instances (verified on
   the four that previously failed) and expands slightly more (it now does the full work
   to prove optimality). A\*, MM, NBS were exactly optimal throughout.
2. **Forward A\* expands the fewest nodes; bidirectional algorithms expand more**, and
   reverse A\* is much worse than forward. This matches the literature (Siag & Shperberg:
   on grid/map-like domains with strong consistent heuristics, unidirectional search is
   often competitive/best and directional asymmetry dominates). Encouraging that the
   toy run already reproduces the expected qualitative behaviour.

### Expansion vs. the must-expand floor (plant01, `mvc` tool)
Average `expanded / MVC` over the harder instances, with **0 lower-bound violations**
(every algorithm expands ≥ MVC, confirming the floor is valid):

| alg | avg exp/MVC |
|---|---:|
| astar | **1.49** |
| nbs | 12.83 |
| bae | 15.92 |
| mm | 25.70 |
| rastar | 40.27 |

On these plant maps the MVC equals the forward-candidate count (forward f<C\* region is
far smaller than backward), so A\* sits near the floor while bidirectional search pays a
large premium. This is a domain-specific result — expect it to shift on sandstone/descent
and with weaker heuristics; the point is the measurement pipeline now works end-to-end.

MM's per-instance time is an outlier (~327 ms vs single-digit ms) — HOG2's MM priority
queue (keyed on `pair<double,double>`) is expensive; worth profiling if MM stays in scope.

---

## 5. Remaining work (in priority order)

1. ~~Investigate BAE\* suboptimality~~ — **done** (gcd rounding; see §4.1).
2. **Zip handling** — either unzip in the fetch step (current) or read `.3dmap.zip` directly.
3. ~~MVC / must-expand floor~~ — **done** (`mvc.cpp`, base admissible case; see §4).
   Future: the tighter *consistency-aware* floor (add the h_C term / max-flow, Shaham 2018 & Siag 2025).
4. **Scale**: `std::vector<bool>` is fine to ~1.3 B voxels (~160 MB); add per-instance
   timeouts and run the full 2000-instance scenarios off the main thread.
5. **BiA\***: pick HOG2's mapping (bidirectional A\* ≈ MM with f-priority, or a dedicated
   class) — currently not wired.
6. **Sandstone (`rev_voxel`) end-to-end test** — loader supports it; download one and confirm.
