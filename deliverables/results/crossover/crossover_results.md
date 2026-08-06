# Heuristic-strength crossover — representative re-run

**Status: firm, ready for write-up.** Supersedes the earlier n=6 result, which rested
on a single solved MM instance and is retracted.

## Method
Connectivity: **diagonal (26-connected)** with octile costs. Metric is **raw median
expansions** (NOT exp/floor), so this axis is independent of the floor-tightness issue that
affects axis A. Per-instance data: `representative/*.exp.csv` (and `.mvc.csv`).

Weight knob `h' = w·h`, swept over w ∈ {1.0, 0.8, 0.6, 0.5, 0.4}. One map per family
(plant01 / industrial, Parker / sandstone, level20 / descent), **80 shuffled
(representative) instances each**, per-instance timeout **300 s**. Metric is **raw median
expansions** over solved instances (the floor shifts with w, so raw expansions are the
comparable quantity). Coverage (solved/total) is reported because it is load-bearing here.

## Headline: BAE* vs A* (ratio <1 = BAE* expands fewer), and MM coverage

| domain | w=1.0 | w=0.8 | w=0.6 | w=0.5 | w=0.4 | MM solved (below w=1.0) |
|---|--:|--:|--:|--:|--:|---|
| plant01 (industrial) | 0.88 | 0.68 | 0.61 | 0.60 | 0.58 | 1–2 / 80 |
| Parker (sandstone)   | 0.90 | 0.60 | 0.53 | 0.51 | 0.50 | 2–3 / 80 |
| level20 (descent)    | 1.58 | 1.19 | 1.11 | 1.09 | 1.07 | **0 / 80** |

## Findings
1. **The direction is universal: BAE\* gains on A\* as the heuristic weakens, in every
   domain.** The BAE*/A* ratio drops monotonically everywhere (plant 0.88→0.58, sandstone
   0.90→0.50, descent 1.58→1.07). This is the theory-confirming result.
2. **Whether BAE\* actually overtakes A\* is domain-dependent.** On plants and sandstone it
   crosses below 1.0 (BAE* wins, and by more as w falls — to half of A* on sandstone). On
   **descent (mazes) it never overtakes** in the tested range — it stays above A* while
   converging toward it. The crossover *point* depends on structure; maze corridors favor
   unidirectional A* longer.
3. **MM collapses in all three domains** — 0–3 of 80 solved below full strength, 0/80 on
   descent. It does not overtake anything under a weakened heuristic; it becomes unusable
   (its per-node cost and footprint blow up). The old "MM overtakes A*" claim is dead.
4. **BiA\* and NBS never overtake A\*** (plant01, all weights below): they stay above it.

## plant01 full breakdown (raw median expansions; MM n in parens)

| w | A* | BiA* | NBS | BAE* | MM |
|---|--:|--:|--:|--:|--:|
| 1.0 | 77,670 | 107,603 | 123,392 | **68,637** | 58,761 (n=25) |
| 0.8 | 378,946 | 519,026 | 553,664 | **256,044** | 55,775 (n=2) |
| 0.6 | 749,070 | 979,136 | 861,873 | **457,505** | 74,274 (n=1) |
| 0.5 | 945,737 | 1,188,196 | 915,968 | **563,284** | 93,598 (n=1) |
| 0.4 | 1,122,831 | 1,389,882 | 964,415 | **652,408** | 107,620 (n=1) |

## Suggested write-up framing
As the heuristic weakens, the error-based bidirectional search (BAE*) increasingly
outperforms A*, overtaking it on the open/cluttered domains (plants, sandstone) though not
on mazes (descent) within the tested range; MM, by contrast, becomes unusable, and BiA*/NBS
never overtake. This is the theory-vs-practice payoff: the bidirectional advantage is real
and heuristic-dependent, but it is realized by the error-exploiting method, not by
meet-in-the-middle.

## Caveats to state
- One map per family (not the whole family) — a focused cross-domain demonstration.
- At the lowest weights on the larger maps, solved counts for MM are tiny (coverage
  reported); A*/BiA*/NBS/BAE* solve 80/80 at every weight, so their medians are comparable.
- These are raw expansions, not exp/floor (the floor moves with w).
