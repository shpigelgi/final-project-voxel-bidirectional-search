# NBS sub-floor: build-flag comparison data

These three files back the paper's claim that NBS's below-floor expansion counts are a
consistency-exploiting class escape (the same rule as our BiA*), not a violation of the
must-expand bound. They are additive; nothing else under `deliverables/results/` was modified.

## The two builds

The driver was compiled twice from identical source, differing only in one preprocessor flag:

- **`bin-nbs` (default)** - HOG2 as shipped, `ADMISSIBLE` **undefined**. In this build NBS
  discards a generated node already closed on the opposite frontier (`NBS.h:382-385`
  `current.Remove(childID)`, and `NBS.h:397-400` `break` / do-not-open). Sound only because a
  consistent heuristic makes the opposite-side g final - the same rule the paper adds to BiA*.
- **`bin-nbs-adm` (`-DADMISSIBLE`)** - the discard branches are `#else`-guarded, so with the flag
  set they are not compiled and NBS keeps every candidate. This is the in-class behaviour.

`ADMISSIBLE` is not set anywhere in `build.sh`; `BDOpenClosed.h:15` is `//#define ADMISSIBLE`
(commented). So the default build is the one used for every number in the paper.

## Files

### `nbs_admissible_test.csv` - the 12 deepest cases (descent/levels1, diagonal)
One row per (build, instance). Columns: `build, tag, instance, alg, expanded, generated, cost,
optimal, time_ms, status, peak_mb, discarded`. `discarded` = opposite-frontier-closed nips
(0 in the `-DADMISSIBLE` build by construction). Instances 9,39,128,166,178,290,305,330,340,344,
352,413 (the ratio<0.5 population).

### `nbs_admissible_batch2.csv` - confirmation across 3 maps and both movement models
Same columns plus `map, config`. level02-nodiag (383,75,154,53,245,140,185,350 - the 127-case
6-connected majority population), level05-diag (119,353), level14-nodiag (483,8).

### `nbs_batch2_mvc.csv` - the epsilon shell on the batch-2 instances
Columns: `map, config, instance, cstar, fwd_cand, bwd_cand, mvc, mvc_eps, pstar`.
- `mvc` = the epsilon-free must-expand floor (g_F+g_B < C*), the **same denominator used in
  `cluster/combined_long.csv`** - it is not recomputed here, it matches that file.
- `mvc_eps` = the epsilon=1 refined cover (g_F+g_B+1 < C*), what MM and NBS actually commit to.
- `pstar` = the offline-optimal fMM split (argmin-tau / C*); 0 or 1 means the optimal cover is
  all-backward / all-forward (lopsided), where the epsilon shell is exactly 0.

## What the data shows

For every one of the 24 instances, `bin-nbs` expands below the floor (0.377-0.834x) and
`bin-nbs-adm` expands above it (1.109-1.465x), at the same optimal cost. Disabling the discard is
the only change. The `nbs_batch2_mvc.csv` shell (mvc - mvc_eps) is at most 0.246% on these
6-connected instances - two orders of magnitude too small to explain 17-43% shortfalls - so those
cases cannot be the epsilon effect, independent of the rebuild. The floor (`mvc`) itself is
unchanged from the released aggregate.

Reproduce: `XFLAGS="-DADMISSIBLE" bash src/build.sh <dir>` for the admissible build; the driver's
`nipped`/`discarded` column reports `NBS::GetNodesDiscarded()`; `mvc --epsilon`-aware column is
`mvc_eps`.
