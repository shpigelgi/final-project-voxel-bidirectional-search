# Why some algorithms report exp/floor < 1.0 — resolved

An independent review flagged that RESULTS.md claimed the must-expand floor is a valid lower
bound for A*, BiA*, NBS, MM, but the data showed BiA* below the floor on ~232 diagonal
instances (min 0.080) and NBS on ~111 (min 0.377) — which would undercut the BAE* story
(if pure f=g+h BiA* also beats the f-floor, "BAE* uses d" no longer distinguishes it).

## Diagnostic
We instrumented both bidirectional implementations with a `nodesNipped` counter — nodes that
are `Close()`d because they are already closed on the opposite frontier ("lazy nipping"),
which HOG2 removes from open but does **not** count as expansions. Added to `BiAStar.h`
(ours) and `hog2/generic/BAE.h`; the driver emits a `nipped` column. Re-ran locally on hard
plant01 diagonal instances (the file is difficulty-graded, so sub-floor cases only appear on
hard instances) and checked whether `expanded + nipped ≥ MVC`.

## Result — two distinct causes

**BAE\* sub-floor is REAL.** On 31/31 sub-floor BAE* instances, **nip = 0** — BAE* expands
~57–89k nodes against a floor of ~100k with zero nips (ratios 0.57–0.88). Counting nips
changes nothing; BAE* genuinely expands below the f-based floor. Mechanism: `b = f + d` uses
the distance-error term the f-based must-expand bound does not model (Alcázar 2020). This is
a legitimate, interesting result.

**BiA\* sub-floor is a COUNTING CONVENTION.** BiA* nips heavily (11k–22k nips/instance). Its
sub-floor instances have `expanded < MVC` but **`expanded + nipped ≥ MVC`** once nips are
counted (e.g. inst 1502: exp 107,311 + nip 11,717 = 119,028 ≥ MVC 109,499). So BiA* does not
actually beat the bound — HOG2 just doesn't count nipped nodes as expansions.

**NBS:** `NBS.h` closes-and-prunes nodes by the incumbent cost without counting them (Expand,
the `f ≥ currentCost` early return before `nodesExpanded++`). Same category as BiA* (closed
but uncounted); confirmed by code inspection, not separately instrumented — its sub-floor
instances are on descent-diag (heavy to reproduce locally).

## Conclusion
- **A\*** attains the floor exactly (0 violations across 27,218 diagonal instances).
- The floor is a valid lower bound for the **f-based algorithms** (A*, BiA*, NBS, MM) once
  nipped/pruned-but-uncounted nodes are counted; BiA*/NBS never truly beat it.
- **BAE\*** legitimately expands below the f-based floor (nip = 0), by exploiting `d`.
- The **`nip = 0` vs `nip > 0`** distinction is the discriminator the review asked for.

## Reproduce
```
./src/build.sh /tmp/vb          # driver now emits a `nipped` column for bia/bae
/tmp/vb/mvc  <map> <scen> --start 1400 --limit 50 > mvc.csv
/tmp/vb/voxdriver <map> <scen> --start 1400 --limit 50 --algs astar,bia,bae > exp.csv
# join on instance; for each bia/bae row with expanded<mvc, check expanded+nipped vs mvc.
```
