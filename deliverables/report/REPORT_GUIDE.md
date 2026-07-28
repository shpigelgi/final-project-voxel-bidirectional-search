# Report-Writing Guide — Bidirectional Heuristic Search on Voxel Maps

A scaffold for writing the final report. It maps each section to the material already
produced (papers, code, results, figures) and says what belongs where. **This is a
guide — it is not the report.** Write in your own voice; cite the artifacts named here.

## The assignment (what must be demonstrated)
From Lior Siag's brief (`instructions/email-from-lior.md`): **run bidirectional search
algorithms on 3D Voxel maps**, connecting the **Warthog voxel benchmark** to the
**bidirectional-search code in HOG2**, running **MM, BAE\*, A\*, BiA\*, GBFS** (+
unidirectional variants) **from both directions**, both **with and without diagonal
movement** (since the domain supports diagonals). Reading order for the theory:
Siag → Eckerle → Holte(MM) → Alcázar(BAE\*) → Shaham ×2 → domain paper.

## One-sentence thesis (the spine of the report)
> On these strong-heuristic 3D voxel domains, **forward A\* is essentially optimal**
> (within ~0–5% of the provable must-expand floor and the fewest-expansions winner on
> ~66% of instances), the bidirectional algorithms pay a **2–6× premium**, and *which*
> algorithm wins is governed by **heuristic strength and directional asymmetry** — not
> by the choice of bidirectional method. A weakened-heuristic sweep shows the expected
> **crossover** where bidirectional search overtakes A\*.

Everything in the report should build toward or qualify this claim.

## Recommended structure

### 1. Introduction & problem statement (~½ page)
What bidirectional heuristic search (BiHS) is and why it might beat unidirectional A\*;
the concrete task (Warthog voxels × HOG2). State the thesis as the question you answer.

### 2. Background / related work (~1–1.5 pages)
Summarize the theory, don't reproduce it. Source: `deliverables/report/background-and-plan.pdf`
(already written and compiled) — lift its structure. Must cover:
- **Must-expand pairs → G_MX → Minimum Vertex Cover** as the *theoretical floor* on
  expansions (Eckerle 2017; Chen/NBS 2017). This is your yardstick — define it here.
- **MM** meet-in-the-middle (`max(f,2g)`, Holte 2017) and **BAE\*** (the `b = f+d`
  error-based priority, Alcázar 2020).
- Consistency & the lower-bound unification (Shaham 2018; Siag & Shperberg 2025), incl.
  the prediction that BiHS helps more when the heuristic is weak.
Cite via the compiled bibliography in `background-and-plan.tex`.

### 3. Domain & method (~1.5 pages)
- **Voxel domain & movement model:** 3D grid; 26-connected (diag) with octile-3D costs
  `{1,√2,√3}` and the **no-corner-cutting** rule; 6-connected (nodiag) unit-cost. File
  format `voxel`/`rev_voxel`. Source: `workspace/src/FINDINGS.md` §domain.
- **What already existed in HOG2 vs. what we built.** Key finding to state plainly:
  HOG2's `Voxels` class is a search stub; the working env is `VoxelGrid`, and we wrote
  a headless `VoxelMap` (loads both formats, both connectivities). BiA\* had no HOG2
  class → we implemented Pohl-style bidirectional A\* (`BiAStar.h`). Source: `FINDINGS.md`.
- **Metric & correctness:** node expansions reported as a ratio to the per-instance
  **MVC floor** (`mvc.cpp`), plus runtime, cost, status. Optimality checked two ways:
  every optimal algorithm agrees on cost per instance, and an **independent legality
  auditor** (`validate.cpp`) confirms 0 illegal/clipping moves.
- **Experimental setup:** BGU HPC (Slurm `cpu` partition), per-instance timeout +
  memory cap, one array task per map×config. Report exact `LIMIT`/`TIMEOUT` used
  (plants 200, sandstone 25, crossover 15; 45–120 s). Reproducible via the
  `bgu-cluster` skill + `cluster/RUNBOOK.md`.

### 4. Results (~2 pages) — lead with figures
Figures are in `deliverables/report/figures/` (PNG for drafts, **PDF for the final**):
- **`exp_over_mvc.{pdf,png}`** — the headline grouped bar: median expansions ÷ floor,
  per algorithm × domain × movement mode. Discuss A\* at ~1.0, the 2–6× bidirectional
  premium, and the narrower spread on sandstone.
- **`winrate.{pdf,png}`** — per-instance fewest-expansions winner (A\* 66%, but a real
  34% for bidirectional). Use this to resist the oversimplified "A\* always wins."
- **`directional_asymmetry.{pdf,png}`** — reverse/forward A\* ratio (plants-diag ~2.9×).
  This is *the* explanatory variable for why MM overpays.
- **`crossover.{pdf,png}`** — expansions vs. heuristic weight; show the point where a
  bidirectional method overtakes A\* as the heuristic weakens (the theory's prediction).
Numbers/tables: `deliverables/results/cluster/{summary.csv, RESULTS.md}` (RESULTS.md
already has the medians table and the written findings — reuse them).
Mention **GBFS** separately (not cost-optimal): ~1% of A\*'s expansions on plants for
~4% longer paths; a worse trade (18%) on sandstone.

### 5. Discussion (~1 page)
Interpret through the theory: strong consistent heuristic ⇒ A\* already near the floor
⇒ little slack for BiHS (Dechter–Pearl / Holte GR2–GR3). Directional asymmetry, not the
algorithm, drives the ranking here. The crossover confirms BiHS's regime is weak
heuristics — consistent with Shaham/Siag. Note MM's cost (its `max(f,2g)` cap +
pair-priority queue) → the only algorithm to time out on the hardest diag instances.

### 6. Limitations & future work (~½ page)
Bounded instance counts (not the full 2000/map); plant04-diag at 100 instances; **descent
domain not run** (large maps); the MVC floor used is the base-admissible one — the tighter
**consistency-aware floor** (Shaham 2018 / Siag 2025) is future work. Front-to-front and
DVCBS/DBBS not evaluated.

### 7. Reproducibility appendix (~½ page)
Point to the `bgu-cluster` skill + `RUNBOOK.md`: `setup.sh` → `fetch-benchmarks.sh` →
conda static build → `submit.sh` → `aggregate.py` → `plot_results.py`. Note the two
cluster quirks that matter for anyone re-running (no system C++ compiler → conda;
GitHub blocked → rsync). Optional: link the interactive 3D visualizer artifact.

## Deliverables checklist
- [ ] Report PDF (this structure) in `deliverables/report/`.
- [x] Background/theory doc — `background-and-plan.pdf`.
- [x] Results tables + written findings — `deliverables/results/cluster/{summary.csv,RESULTS.md}`.
- [x] Figures — `deliverables/report/figures/*.pdf`.
- [x] Code (loaders, all algorithms, MVC, validator, visualizer) — `workspace/src/`.
- [x] Reproducible cluster pipeline — `workspace/cluster/` + `.claude/skills/bgu-cluster/`.
- [ ] (Optional) descent-domain results; larger/uniform instance counts.

## Writing reminders
- Report **medians**, not means — a few very hard instances skew the mean (see the large
  mean/median gap in `summary.csv`).
- Always say **both** movement modes and note the nodiag caveat: the scenario optimal cost
  is diagonal-only, so nodiag optimality is verified by cross-algorithm agreement.
- Keep the honest framing: this is an **A\*-favorable regime**; the interesting science is
  *why*, and *when it flips* (the crossover), not a leaderboard.
