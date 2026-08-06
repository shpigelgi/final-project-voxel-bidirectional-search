# Why some algorithms report exp/floor < 1.0 — resolved (corrected)

An independent review flagged that RESULTS.md claimed the must-expand floor is a valid lower
bound for A*, BiA*, NBS, MM, while the data showed BiA*, NBS, and BAE* all below it on some
instances. This note records the investigation and its **corrected** conclusion. (An earlier
version of this note wrongly concluded "BiA* = uncounted nipping, BAE* = real d-term"; that held
only for near-threshold instances and is retracted.)

## What we tested
Instrumented a `nodesNipped` counter in BiAStar.h and hog2/generic/BAE.h (nodes closed because
already closed on the opposite frontier — removed from open but not counted as expansions), had
the driver emit a `nipped` column, rebuilt, and checked `expanded + nipped` vs. the computed MVC
on hard diagonal instances.

## Findings
- **Near-threshold cases (exp/floor ≈ 0.95–1.0):** BiA* nips 11k–22k nodes and
  `expanded + nipped ≥ MVC` — a counting convention, consistent with the floor.
- **Extreme cases falsify that explanation.** plant02-diag inst 1886: BiA* is optimal, expands
  85,917, nips only 3,144 → 89,061 total, against a computed floor of **199,454** (ratio 0.447).
  Counting nips does not close a 2× gap. Since an optimal front-to-end algorithm's expanded set is
  a valid vertex cover, the **true MVC ≤ ~89k, so the computed floor over-states it ~2×** here.
- BAE* sub-floor instances have nip = 0 yet expand well below the floor — same phenomenon, not a
  separate d-term effect distinguishable from bound-looseness.

## Root cause: the pairwise floor is loose, not a coding bug
The threshold scan in `mvc.cpp` is correct (skipping duplicate g-values is valid), and the graph
`u~v iff g_F(u)+g_B(v) < C*` is a chain graph, so the threshold form equals the true MVC **of
that graph** (König). The looseness is in the **bound/graph itself**: the pairwise summed-g
condition over-counts what a front-to-end algorithm using individual f-bounds actually needs.
This is the individual-bounds gap of \citealt{alcazar2020unifying}. On inst 1886 the MVC equals
the all-forward extreme (`MVC = fwd_cand = 199,454`); A* attains it, the bidirectional algorithms
beat it because the pairwise bound is not tight.

## Consequence
`exp/floor < 1.0` means "below this specific pairwise lower bound," **not** "below the optimum."
A* attains the bound; BiA*/NBS/BAE* dip below it where the bound is loose. RESULTS.md axis A is
framed accordingly. A tight per-instance bound would require the full individual-bounds/MEP
machinery — future work, not a patch.

## Reproduce
```
./src/build.sh /tmp/vb   # driver emits a `nipped` column for bia/bae; needs hog2-patches/add_bae_nipped.py
/tmp/vb/mvc <map> <scen> --start 1850 --limit 90 > mvc.csv
/tmp/vb/voxdriver <map> <scen> --start 1850 --limit 90 --algs astar,bia,bae > exp.csv
# join on instance; for bia rows with expanded<mvc, check expanded+nipped vs mvc.
# plant02 hard tail contains the extreme (e.g. inst 1886).
```
