# Cluster benchmark results (first sweep)

Produced on the **BGU HPC** Slurm cluster (`cpu` partition) via `cluster/submit.sh`.
Two domains, both movement models, all seven algorithms. Metric of record is
**node expansions relative to the per-instance must-expand floor** (MVC), reported as
the **median `exp/MVC`** (robust to the trivial `MVC=0` instances and to a few very hard
outliers that skew the mean). Full data: `combined_long.csv`; per-group: `summary.csv`.

Run config: plants `LIMIT=200`, sandstone `LIMIT=25`, per-instance timeout 45–60 s,
8 GB/child, 1 CPU/task, one array task per map×config.

## Correctness
**Optimality consistency: 100% on every group** — the optimal algorithms (A\*, reverse
A\*, MM, BiA\*, BAE\*, NBS) return the identical cost on every instance:
547/547 and 1000/1000 (plants diag/nodiag), 275/275 and 275/275 (sandstone diag/nodiag).
No wrong-cost results across ~14.7k rows.

## Expansions vs. the floor (median exp/MVC — lower is better; 1.0 = optimal)

| domain / mode | A\* | BiA\* | NBS | BAE\* | MM | rev-A\* |
|---|--:|--:|--:|--:|--:|--:|
| plants · diag     | **1.00** | 2.00 | 2.25 | 3.69 | 5.36 | 4.76 |
| plants · nodiag   | **1.02** | 2.03 | 3.10 | 3.50 | 3.63 | 2.82 |
| sandstone · diag  | **1.00** | 1.98 | 2.01 | 2.41 | 2.82 | 2.69 |
| sandstone · nodiag| **1.05** | 2.01 | 2.38 | 2.63 | 2.96 | 2.51 |

## Findings
- **Forward A\* is essentially optimal** on these voxel domains — it sits within ~0–5% of
  the theoretical must-expand floor in every configuration. On grid/map-like domains with
  a strong consistent heuristic, the unidirectional search already expands nearly the
  minimal set; this matches the literature (Siag & Shperberg; Holte et al.).
- **The bidirectional algorithms pay a 2–6× premium** here. Among them **BiA\*** is
  consistently cheapest (~2×) and **MM** the most expensive (frontier-balancing overhead
  + the `max(f,2g)` cap forcing extra expansion). The gap is **narrower on sandstone**
  (porous, more random-grid-like — where bidirectional theory predicts more benefit) than
  on the blocky plants.
- **GBFS** is not cost-optimal (reported as `subopt`); it expands far fewer nodes but its
  paths are longer — a speed/quality axis, not comparable to the MVC floor.
- **Timeouts:** only **MM** hit the wall-clock cap, on the hardest `diag` instances (28%
  on plants — driven by the largest map, plant04 — and 24% on sandstone). Its per-node
  cost (pair-priority queue) makes it slow on the largest searches; every other optimal
  algorithm finished 100% of instances.
- **Runtime:** A\* is fastest (~10–40 ms/instance median), MM slowest (seconds on hard
  diag instances). Reverse A\* expands the most on plants but is competitive on sandstone
  — directional asymmetry is domain-specific.

## Heuristic-strength crossover (the "when does bidirectional win" test)
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
overtake; plain BiA\*/NBS do not. See `deliverables/report/figures/crossover.pdf`.

## Caveats / scope
- Plants used the first 200 scenario instances/map (plant04, the largest map, 100),
  sandstone the first 25 (sandstone searches are far heavier — 10⁴–10⁵ expansions — so a
  smaller, still-representative sample keeps cluster use responsible). All 5 plant maps and
  all 11 sandstone maps ran in both modes. Descent not yet run (large map fetch).
- `diag` MM/BAE\* on the biggest plant maps are very slow at deep instance indices; a
  longer timeout would convert some MM timeouts into completed runs but cost far more
  compute. The medians above are over completed (`status=ok`) instances.
