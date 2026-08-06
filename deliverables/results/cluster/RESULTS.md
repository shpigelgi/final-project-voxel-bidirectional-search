# Cluster benchmark results (representative sample)

Three domains (industrial plants, sandstone, descent), both movement models, all seven
algorithms, both directions, on a seeded-shuffle representative sample with a bootstrap-median
±3% CI stopping rule (see `convergence_report.txt`). Per-instance data: `combined_long.csv`
(384,227 rows). Metric medians: `summary.csv`.

## Reading the floor axis correctly (important)
Axis A compares node expansions to the **must-expand lower bound** — the MVC of the
must-expand graph under the pairwise condition `g_F(u)+g_B(v) < C*` (Shaham et al. 2017/2018).
**This bound is valid but not tight.** It is the bound implied by *summed* g-values; front-to-end
algorithms that exploit individual f-bounds can expand fewer nodes than it
(\citealt{alcazar2020unifying}). Empirically:
- **A\*** attains the bound exactly — it expands the full forward `f<C*` contour, which equals
  the bound's all-forward cover (verified: 0 of 27,218 diagonal A* instances below 1.0).
- **The bidirectional algorithms dip below it** on the harder instances, by a little to a lot
  (down to ~0.04× in extreme cases). This is *not* "better than optimal": it means the pairwise
  bound is loose for those algorithms/instances. We confirmed this directly — e.g. plant02-diag
  inst 1886: BiA* is optimal and processes 89,061 nodes (incl. nips) against a computed floor of
  199,454, so the true minimum is ≤ 89k and the floor over-states it ~2×.

So **read `exp/floor` as distance from this specific pairwise bound, not from the per-instance
optimum**: values > 1 quantify avoidable work relative to the bound; values < 1 show the bound is
not tight for that algorithm. A tight bound would need the full individual-bounds machinery
(future work). We did **not** claim, and the data does not support, that any algorithm beats the
optimum.

## (A) Expansions vs. the (pairwise) floor — median exp/MVC

| domain / mode | A* | BiA* | NBS | BAE* | MM | rev-A* |
|---|--:|--:|--:|--:|--:|--:|
| plants · diag      | **1.00** | 1.62 | 1.69 | 1.57 | 3.39† | 1.73 |
| plants · nodiag    | **1.06** | 2.03 | 3.41‡ | 3.62 | 4.60‡ | 3.04‡ |
| sandstone · diag   | **1.00** | 1.54 | 1.66 | 0.95 | 2.63† | 1.35 |
| sandstone · nodiag | **1.00** | 1.73 | 1.56 | 1.25 | 1.63 | 1.58 |
| descent · diag     | **1.00** | 1.63 | 1.58 | 1.47 | 2.74† | 1.38 |
| descent · nodiag   | **1.00** | 1.78 | 1.72 | 1.72 | 1.84 | 1.47 |

† MM median over solved instances only — MM times out on most diagonal instances (see C).
‡ Did not reach ±3% CI (heavy-tailed / timeout-limited); reported with wider CI, see
`convergence_report.txt`. plants-nodiag has ~83% MVC=0 exclusions, leaving few usable instances.

**Findings.** A* sits at the bound; the bidirectional algorithms are mostly ~1.4–1.8× on diagonal
maps (BAE* lowest, sometimes below the bound). The old easy-biased sample inflated this to
2–6×; representative sampling both softens the premium and exposes the bound's looseness. Note
the bidirectional medians are over *solved* instances and those algorithms have non-trivial
diagonal timeout rates (below), so their medians are mildly optimistic (survivorship) — this
applies to BAE* too, not only MM.

## (C) Memory (median peak MB) and robustness

| domain / mode | A* | BiA* | NBS | BAE* | MM | rev-A* |
|---|--:|--:|--:|--:|--:|--:|
| plants · diag      |  96.8 | 110.2 | 107.3 | 103.6 | 240.6 | 184.3 |
| plants · nodiag    |  11.4 |  11.4 | 104.1 |  11.4 |  42.3 |  13.6 |
| sandstone · diag   | 181.1 | 208.1 | 193.3 | **152.4** |  28.0§ | 207.0 |
| sandstone · nodiag | 101.3 | 143.3 | 127.1 | **96.8** | 147.8 | 181.3 |
| descent · diag     | 368.4 | 410.6 | **363.7** | 407.2 | 378.5 | 430.1 |
| descent · nodiag   | **270.5** | 364.4 | 318.6 | 359.4 | 410.7 | 403.0 |

§ MM's low sandstone-diag memory is survivorship (it only solves the ~2% easiest instances).
A* is the lightest in most groups but **not all** (descent-diag NBS < A*; sandstone BAE* < A*).
**Memory-exhaustion (`fail`) rate is 0 in every group** — no run hit the address-space cap.

**Robustness / timeouts.** "Only MM times out" is **false** — correcting an earlier claim.
Overall timeout rate by algorithm: **MM 45.1%, BAE* 1.28%, NBS 1.21%, BiA* 0.91%, rev-A* 0.60%,
A* 0.08%.** It concentrates on diagonal maps; on **descent-diag: MM 94%, BAE* 4.6%, BiA* 3.2%,
NBS 3.1%, rev-A* 2.0%, A* 0.3%.** So MM dominates but is not alone, and BAE*'s sub-floor result is
computed with its hardest ~4.6% of descent-diag instances removed.

## (B) Time
A* fastest; MM slowest per node. Across **302,358** solved runs the wall-time distribution is:
median 3.0 s, p99 **38.4 s**, p99.9 **54.3 s**; **0.078%** (235 runs) finish within 5 s of the
60 s cap. So the cap is ~1.6× the p99 (not 2×, correcting an earlier figure), and few solved runs
are near it — but this is over *solved* runs by construction, so it bounds the budget from below,
not a bimodality claim.

## (D) Direction
Reverse vs. forward A* differs by domain (`figures/directional_asymmetry.pdf`). Caveat: the
plants-nodiag rev-A* figure (3.04) did **not** converge to ±3% (heavy-tailed); treat it as
indicative.

## (E) Heuristic-strength crossover — raw expansions (unaffected by the floor issue)
This axis uses **raw median expansions**, not exp/floor, so it is independent of the floor's
looseness. Diagonal (26-connected) mode. Weight sweep `h'=w·h`, one map per family, 80 shuffled
instances, 300 s timeout. BAE*/A* ratio (<1 = BAE* fewer):

| domain | w=1.0 | w=0.8 | w=0.6 | w=0.5 | w=0.4 | MM solved (<w=1) |
|---|--:|--:|--:|--:|--:|---|
| plants    | 0.88 | 0.68 | 0.61 | 0.60 | 0.58 | 1–2/80 |
| sandstone | 0.90 | 0.60 | 0.53 | 0.51 | 0.50 | 2–3/80 |
| descent   | 1.58 | 1.19 | 1.11 | 1.09 | 1.07 | 0/80 |

BAE* increasingly beats A* as the heuristic weakens (overtaking on plants+sandstone, converging
toward it on descent). MM collapses (0–3/80 solved below full strength); sensitivity confirms
intrinsic (still 0–2% at 5× budget). BiA*/NBS never overtake. Full breakdown +
per-instance data: `../crossover/`.

## GBFS (speed/quality reference, not floor-comparable)
GBFS is not cost-optimal: median cost/C* = **1.337** (paths ~34% longer), median **27,655**
expansions. Reported as a speed/quality trade-off, excluded from the floor tables.

## Sample sizes, exclusions, and gaps
- Instances/map: descent ~500, sandstone ~500, plants ~1500 (both modes). Full data in
  `combined_long.csv`; per-group MVC=0 counts in `mvc0_counts.csv` (plants-nodiag 83.4% — these
  are heuristic-exact, h(s)=C*, excluded from exp/floor).
- **106 descent-diag instances are missing** (level10 429/500, level16 465/500): those chunks hit
  the 12 h node wall (TIMEOUT at task level), so a slice of the hardest instances is absent for
  all algorithms. Descent-diag n is therefore 14,394, not 14,500.
- The four `NEED_EXTEND` groups stopped at round 1 / 1500-per-map, not because a budget ran out;
  their unconverged cells are heavy-tailed and might converge with more instances.
- Validation counts (~1.8M successors / ~58k edges, 0 illegal) are from an earlier subset, not
  refreshed; the 0-illegal legality conclusion is unaffected.
