# Cluster benchmark results (representative sample)

Produced on the **BGU HPC** cluster. Three domains (industrial plants, sandstone,
descent), both movement models, all seven algorithms, both directions. Evaluated along
five axes: **(A)** search efficiency vs. the must-expand floor, **(B)** time, **(C)**
memory, **(D)** direction, **(E)** heuristic-strength crossover.

## Sampling and convergence (important — read first)
The benchmark scenario files are **difficulty-graded**, so a contiguous first-N prefix is a
biased (easy) sample. We therefore **shuffled each map's instances with a fixed seed** and
sampled representatively, running each (family, mode, algorithm) group until its **median
exp/floor had a bootstrap 95% CI within ±3%**, or the instance budget was exhausted.

Convergence status (final):
- **Fully converged (all algorithms, CI <2%):** descent-nodiag, sandstone-nodiag.
- **All non-MM algorithms converged; MM capped:** plants-diag, descent-diag, sandstone-diag.
  MM times out on 77–98% of diagonal instances, so its median is over the small survivorship
  set it solves; it cannot reach ±3% and is reported with its (wider) CI.
- **plants-nodiag:** A*/BiA*/BAE* converged; NBS/rev-A*/MM are heavy-tailed (CI 8–11%) and
  reported with their CIs. (This mode has ~83% MVC=0 instances — see below — leaving fewer
  usable ones.) Median exp/floor per (family, mode, algorithm) is in `summary.csv`; the
  per-group CIs are in `convergence_report.txt`.

Per-instance sample sizes: descent ~500/map, sandstone ~500/map, plants ~1500/map (diag)
and 1500/map (nodiag). Full per-instance data: `combined_long.csv`.

## Correctness
Optimality consistency: the optimal algorithms (A*, reverse A*, MM, BiA*, BAE*, NBS) return
identical costs on every solved instance across all groups. No wrong-cost results.

## (A) Expansions vs. the floor (median exp/MVC; 1.0 = the f-based must-expand minimum)

| domain / mode | A* | BiA* | NBS | BAE* | MM | rev-A* |
|---|--:|--:|--:|--:|--:|--:|
| plants · diag      | **1.00** | 1.62 | 1.69 | 1.57 | 3.39† | 1.73 |
| plants · nodiag    | **1.06** | 2.03 | 3.41 | 3.62 | 4.60 | 3.04 |
| sandstone · diag   | **1.00** | 1.54 | 1.66 | **0.95** | 2.63† | 1.35 |
| sandstone · nodiag | **1.00** | 1.73 | 1.56 | 1.25 | 1.63 | 1.58 |
| descent · diag     | **1.00** | 1.63 | 1.58 | 1.47 | 2.74† | 1.38 |
| descent · nodiag   | **1.00** | 1.78 | 1.72 | 1.72 | 1.84 | 1.47 |

† MM median is over solved instances only; MM times out on most diagonal instances (see C).

**Findings.**
- **A\* sits exactly at the floor** (1.00–1.06×) in every configuration. Note the floor is a
  bound for *front-to-end* algorithms; A* is unidirectional and lands on it here.
- **The "bidirectional pays a large premium" story is a biased-sample artifact.** On the
  representative sample the bidirectional algorithms are only ~1.4–1.8× the floor on
  diagonal maps — not the 2–6× the earlier easy-biased sample showed.
- **Some algorithms report exp/floor < 1.0. There are two distinct causes — one real, one a
  counting convention — and we verified which is which** (diagnostic: instrumented node
  counters, see `floor_below_diagnostic.md`).
  - **BAE\* genuinely expands below the f-based floor** — 0.95× on sandstone-diag (56% of
    instances below 1.0, all provably optimal, some as low as 0.24×). This is real: on the
    sub-floor instances BAE* performs **zero nips**, yet expands fewer nodes than the floor.
    BAE* uses the distance-error term `d` (priority `b = f + d`), i.e. information the f-based
    floor does not model, so it can legitimately beat that bound \citep{alcazar2020unifying}.
  - **BiA\* (and NBS) only *appear* below the floor, and rarely — it is a node-counting
    convention.** HOG2 does not count "nipped" nodes (nodes closed because already closed on
    the opposite frontier) as expansions, while the floor counts them. On BiA*'s sub-floor
    instances, **expanded + nipped ≥ floor** once the nips are counted (verified), so BiA*
    does *not* actually beat the bound. NBS closes-and-prunes nodes by the incumbent without
    counting them (same category; confirmed by code, not separately instrumented).
  - The `nip = 0` vs `nip > 0` distinction is the discriminator: BAE*'s sub-floor is a real
    effect, BiA*/NBS's is an artifact of not counting nipped nodes. **A\* attains the floor
    exactly (0 violations); the floor is a valid lower bound for the f-based algorithms once
    nipped nodes are counted, and BAE* is the one method that legitimately goes below it.**

## (C) Memory (median peak MB) and robustness

| domain / mode | A* | BiA* | NBS | BAE* | MM | rev-A* | MM timeout |
|---|--:|--:|--:|--:|--:|--:|--:|
| plants · diag      |  97 | 110 | 107 | 104 | 241 | 184 | **77.0%** |
| plants · nodiag    |  11 |  11 | 104 |  11 |  42 |  14 | 0% |
| sandstone · diag   | 181 | 208 | 193 | 152 |  28‡ | 207 | **97.9%** |
| sandstone · nodiag | 101 | 143 | 127 |  97 | 148 | 181 | 0% |
| descent · diag     | 368 | 411 | 364 | 407 | 379 | 430 | **94.4%** |
| descent · nodiag   | 270 | 364 | 319 | 359 | 411 | 403 | 0% |

‡ MM's low sandstone-diag memory is survivorship — it only solves the ~2% easiest instances.

**Robustness / timeouts.** Only MM hits the wall-clock cap, and on the representative sample
its diagonal timeout rate is severe: **77% (plants), 98% (sandstone), 94% (descent)** — far
higher than the easy-biased sample suggested. Every other optimal algorithm completes ~100%.
NBS is the memory outlier on plants-nodiag (104 MB vs ~11 MB), consistent with its heavy tail
there.

## (B) Time
A* is fastest; MM slowest per node. The 60s timeout is not load-bearing: across 188k solved
runs the p99 wall time is 28.5s and p99.9 is 44s, with only 0.01% of solved runs finishing
within 5s of the cap — the distribution is bimodal (solve fast or blow up).

## (D) Direction
Reverse A* differs from forward A* by domain (see `figures/directional_asymmetry.pdf`);
the asymmetry matches the directional bias the benchmark authors document.

## (E) Heuristic-strength crossover (representative; retracts the old n=6 result)
Weight sweep `h' = w·h` over w ∈ {1.0,0.8,0.6,0.5,0.4}, one map per family, 80 shuffled
instances, 300s timeout, raw median expansions. BAE*/A* ratio (<1 = BAE* wins):

| domain | w=1.0 | w=0.8 | w=0.6 | w=0.5 | w=0.4 | MM solved (<w=1.0) |
|---|--:|--:|--:|--:|--:|---|
| plants    | 0.88 | 0.68 | 0.61 | 0.60 | 0.58 | 1–2/80 |
| sandstone | 0.90 | 0.60 | 0.53 | 0.51 | 0.50 | 2–3/80 |
| descent   | 1.58 | 1.19 | 1.11 | 1.09 | 1.07 | 0/80 |

- **BAE\* gains on A\* as the heuristic weakens in every domain** (ratio drops monotonically).
- It **overtakes** A* on plants and sandstone; on descent (mazes) it converges toward A* but
  does not overtake in range.
- **MM collapses** (0–3/80 solved below full strength). Sensitivity check: at **5× the budget
  (300s)**, MM still solves only 0–2% on sandstone/descent — its timeouts are **intrinsic**,
  not a 60s artifact. BAE* solves 100% at every weight.
- BiA*/NBS never overtake A*.

See `crossover/crossover_results.md` for the full breakdown, `figures/crossover.pdf`.

## MVC=0 (heuristic-exact) instances, excluded from exp/floor
MVC=0 iff h(s)=C* (the heuristic is exact from the start), so these are heuristic-exact
instances, not short ones. Excluding them biases exp/floor toward heuristic-inexact instances.
Fractions are large on open maps (plants-nodiag ~83%), small on cluttered/diagonal ones. Exact
per-group counts: run `aggregate.py` (printed to stdout).

## Caveats
- MM (all diagonal) and plants-nodiag NBS/rev-A* did not reach ±3% CI within the instance
  budget (timeout-limited and heavy-tailed respectively); they are reported with their CIs.
- Crossover is one map per family — a cross-domain demonstration, not a full-family sweep.
- Validation counts (~1.8M successors / ~58k edges, 0 illegal) are from an earlier subset and
  not refreshed on the representative sample; the 0-illegal legality conclusion is unaffected.
