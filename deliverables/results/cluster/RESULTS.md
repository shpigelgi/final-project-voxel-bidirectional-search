# Cluster benchmark results

Produced on the **BGU HPC** Slurm cluster (`cpu` partition). Three domains
(industrial plants, sandstone, descent), both movement models, all seven algorithms,
run in both directions. We evaluate along the axes set out in the report methodology:
**(A)** search efficiency vs. the must-expand floor, **(B)** time, **(C)** memory,
**(D)** direction, and **(E)** the heuristic-strength crossover.

Metric of record for (A) is **node expansions relative to the per-instance must-expand
floor** (MVC), reported as the **median `exp/MVC`** (robust to trivial `MVC=0` instances
and to a heavy tail that skews the mean). Full data: `combined_long.csv`; per-group:
`summary.csv`.

Run config: plants `LIMIT=200`, sandstone `LIMIT=25`, descent `LIMIT=250`, per-instance
timeout 60 s, 8 GB/child, 1 CPU/task. Peak resident memory (`peak_mb`) is logged per run
for all three domains. Descent diag maps were split into 5 instance-chunks per map so the
hardest (level16) fits the node time limit; chunks are stitched back before aggregation.

## Correctness
**Optimality consistency: 100% on every group** — the optimal algorithms (A\*, reverse
A\*, MM, BiA\*, BAE\*, NBS) return the identical cost on every completed instance, across
all three domains and both modes. No wrong-cost results across ~64k rows.

## (A) Expansions vs. the floor (median exp/MVC — lower is better; 1.0 = optimal)

| domain / mode | A\* | BiA\* | NBS | BAE\* | MM | rev-A\* |
|---|--:|--:|--:|--:|--:|--:|
| plants · diag     | **1.00** | 2.00 | 2.21 | 3.78 | 5.12 | 4.64 |
| plants · nodiag   | **1.02** | 2.03 | 3.10 | 3.50 | 3.63 | 2.82 |
| sandstone · diag  | **1.00** | 1.98 | 2.01 | 2.41 | 2.75 | 2.69 |
| sandstone · nodiag| **1.05** | 2.01 | 2.38 | 2.63 | 2.96 | 2.51 |
| descent · diag    | **1.00** | 1.99 | 2.00 | 2.34 | 3.26 | 2.26 |
| descent · nodiag  | **1.01** | 2.00 | 2.21 | 2.75 | 3.11 | 2.48 |

## (C) Peak memory (median MB per instance)

| domain / mode | A\* | BiA\* | NBS | BAE\* | MM | rev-A\* |
|---|--:|--:|--:|--:|--:|--:|
| plants · diag     | 23.5 | 29.4 | 36.8 | 41.9 | **69.8** | 49.2 |
| plants · nodiag   | 10.9 | 10.9 | 40.1 | 10.9 | 22.0 | 13.5 |
| sandstone · diag  |  9.8 | 10.8 |  9.8 | 10.8 | **16.8** | 10.8 |
| sandstone · nodiag|  8.8 |  8.8 | 10.8 |  8.8 | 10.8 |  8.8 |
| descent · diag    | 88.3 |111.1 |108.0 |133.3 | **252.4** |134.8 |
| descent · nodiag  | 77.7 | 89.9 |102.8 |107.3 |121.0 |113.1 |

## Findings
- **(A) Forward A\* is essentially optimal** on all three voxel domains — within ~0–5% of
  the theoretical must-expand floor in every configuration. With a strong consistent
  heuristic the unidirectional search already expands nearly the minimal set; this matches
  the literature (Siag et al.; Holte et al.).
- **The bidirectional algorithms pay a 2–5× premium** here. **BiA\*** is consistently
  cheapest (~2×) and **MM** the most expensive (frontier-balancing overhead + the
  `max(f,2g)` cap forcing extra expansion). The gap is narrower on sandstone and descent
  than on the blocky plants.
- **(C) Memory tracks (A), and MM is the outlier in every diagonal config** — plants diag
  69.8 MB vs A\*'s 23.5 (~3×); descent diag **252 MB** vs 88 (~2.9×), with the hardest
  instances reaching 680–900 MB. Same story as its timeouts: MM does not merely expand
  more, its paired open list makes each node dearer in both time and space. See
  `figures/memory.pdf`.
- **(B) Runtime.** A\* is fastest; MM slowest. Per-expansion cost separates search work from
  overhead: MM costs ~21 µs/expansion vs ~2.5–5 µs for the others (~6–8×), which is *why*
  it is the one that times out. Wall-clock reflects our implementation too, so we read it
  as a practical, not a theoretical, measure.
- **(C) Robustness / timeouts.** Only **MM** hits the wall-clock cap, on the hardest `diag`
  instances: **27%** on plants, **15%** on sandstone, **69%** on descent (mazes) — every
  other optimal algorithm completes ~100%. MM's descent-diag median above is therefore over
  the ~31% it finished (survivorship — the easy instances).
- **(D) Direction.** Reverse A\* expands the most on plants but is competitive on sandstone
  and descent — directional asymmetry is domain-specific (see `directional_asymmetry.pdf`).
- **GBFS** is not cost-optimal (`subopt`): far fewer expansions but longer paths — a
  speed/quality axis, not comparable to the floor.

## (E) Heuristic-strength crossover (the "when does bidirectional win" test)
Sweeping a weakened heuristic `h' = w·h` (still admissible) on plant01's non-trivial
instances directly tests the theory's central claim. Median expansions:

| w (→ weaker) | A\* | MM | BAE\* | BiA\* | NBS |
|---|--:|--:|--:|--:|--:|
| 1.00 (full octile) | **801** | 19 613 | 17 218 | 1 597 | 7 395 |
| 0.60 | 215 578 | **79 753** | 132 233 | 306 903 | 297 017 |
| 0.40 | 314 929 | (timeout) | **110 784** | 286 395 | 211 124 |

**A\* goes from best (801) to worst-of-pack as the heuristic weakens, and MM then BAE\*
overtake it** — exactly the regime the papers predict for bidirectional search. Nuance:
it is specifically the meet-in-the-middle (MM) and error-exploiting (BAE\*) methods that
overtake; plain BiA\*/NBS do not. See `figures/crossover.pdf`.

## Caveats / scope
- Sample sizes: plants first 200 instances/map, sandstone 25 (its searches are far heavier,
  10⁴–10⁵ expansions), descent 250. All maps in each family ran in both modes; descent is
  complete (level16-diag, which previously overran the node limit, now finishes as 5 chunks).
- `diag` MM on the biggest maps is dominated by wall-clock timeouts; the medians are over
  completed (`status=ok`) instances, so MM's diag figures reflect the easier instances it
  finished.
