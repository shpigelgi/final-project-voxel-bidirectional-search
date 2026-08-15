# Why some algorithms report exp/floor < 1.0 — resolved (verified)

An independent review flagged that RESULTS.md treated the must-expand floor as a valid lower
bound for A*, BiA*, NBS, MM, while the data showed several algorithms below it on some instances.
This note records the final investigation and its verified conclusion. Two earlier conclusions in
this file are retracted and explained below, so the reasoning trail is honest.

## The floor is computed correctly (verified three ways)
The floor is the MVC of the pairwise must-expand graph: forward candidates `f_F(u) < C*`, backward
candidates `f_B(v) < C*`, edge iff `g_F(u)+g_B(v) < C*` (Eckerle 2017 Thm 6 / Shaham et al. 2017).
- The candidate g-multisets equal the true A*/reverse-A* optimal contours. On plant02-diag inst
  1886: A* expands 199,456 ≈ fwd_cand 199,454, reverse-A* 311,047 ≈ bwd_cand 311,041.
- The threshold scan attains the true minimum vertex cover. Checked against a direct
  maximum-matching MVC on 3,000 random chain graphs (`verify_mvc_scan.py`, 0 mismatches), and
  directly on inst 1886: the scan's 199,454 is the true minimum over all breakpoints (the balanced
  cover at τ=C*/2 is 254,225, larger).
- C* agrees between floor and runs.

So the floor is not over-computed and the scan is not the issue. Both earlier suspicions —
"uncounted nipping" and later "the floor over-states ~2×" — are **retracted**.

## The floor is a lower bound only for in-class (admissible front-to-end DXBB) algorithms
Eckerle's must-expand result binds algorithms in the DXBB class (deterministic, expansion-based,
black-box, front-to-end). Our algorithms split cleanly:

- **A* / reverse-A*** attain the floor exactly. A* expands the full forward `f<C*` contour, which
  is the bound's all-forward cover (0 of 27,218 diagonal A* instances below 1.0).
- **MM** is in-class with no escape, and it sits **at** the floor. MM expands exactly the set
  `{max(f,2g) < C*}` = `{f<C* and g<C*/2}` on each side, which is the floor's threshold cover at
  τ=C*/2, `S = N_F(C*/2)+N_B(C*/2)`. Because the scan considers τ=C*/2, `MVC ≤ S`, so `MM ≥ MVC`
  up to a boundary gap. Across all cluster data MM's sub-bound ratios are **≥ 0.9759 (median
  0.998)** — 73 cases, all a fraction of a percent. Those dips occur only on *balanced* instances
  where the MVC's minimum lands at τ=C*/2 (so MVC = S) and MM's count of the `g<C*/2` box falls a
  hair short at the strict `g = C*/2` boundary (a rounding effect, wider on diagonal maps where C*
  is irrational). This is the floor being **tight** for MM, not loose.
  - Verified directly on the **worst MM case in the whole run**, reproduced exactly by re-deriving
    the seeded shuffle (seed 20260802): plant02-diag shuffled inst 292. MM/MVC = **0.9759**,
    matching the cluster to four decimals. The floor is sound there — A* = 19,383 vs fwd_cand
    19,379, rev-A* = 20,008 vs bwd_cand 20,006 (candidates equal the contours), C* agrees. It is a
    balanced instance, so `MVC = 18,532 ≈ S = 18,537` (the τ=C*/2 cover is the minimum). MM = 18,085
    = `S − 452`, i.e. it expands the τ=C*/2 box minus a 2.4% `g≈C*/2` boundary sliver. Every other
    MM case is tighter (≥ 0.9759), and integer-cost nodiag maps are tighter still (≥ 0.9949), the
    crisp-boundary version of the same effect (BSG-nodiag inst 69: MM 570,804 vs S 572,282, 0.26%).
- **BiA* and BAE*** dip far below the floor (BiA* to ~0.07×, BAE* to ~0.02×) via a candidate/class
  escape: BiA*'s consistency-based nipping (discarding a node already closed on the opposite
  frontier) plus its individual-f termination `U ≤ max(fmin_F, fmin_B)`, and BAE* likewise. These
  take them outside the strict admissible-DXBB class, so the bound does not bind them and their
  sub-bound scores are expected by our own argument, not evidence about the floor. The earlier
  "floor over-states ~2×" claim was drawn from BiA* inst 1886 and is retracted for exactly this
  reason: it measured an out-of-class algorithm. (Note: on BiA* inst 1886 the 3,144 nips do not
  quantitatively account for the ~110k gap; the individual-f termination is the larger mechanism.)
- **NBS** is the one genuinely open case. It dips to 0.377 (median 0.858, 297 instances), which is
  two orders of magnitude past MM's boundary effect, so MM's explanation does not touch it. NBS is
  admissible DXBB (Eckerle present it as such), has no nipping, and its only uncounted closes are
  incumbent-pruned at f ≥ C* — non-candidates outside the cover — so it is not undercounting
  relative to the MVC. And the floor is not inflated on its sub-bound instances: on the exact
  instances where NBS < 0.5, **A* attains the floor** (A* exp/floor median 1.03, min 1.00, 0 of 12
  below 1.0), so A* corroborates the MVC on those very instances. NBS below a corroborated floor,
  while in the class and not undercounting, is a genuine inconsistency reported as a limitation.
  The leading unverified hypothesis (analogue of the BiA* termination point): NBS's termination on
  C_lb reaching the incumbent may fire before its expanded set covers G_MX. All NBS<0.5 cases are
  on descent maps, which are not available locally, so this could not be tested with the
  three-number check; it is the one thing to chase if the cluster maps return.

## Consequence for the paper
`exp/floor` is distance from this specific pairwise lower bound, and the algorithms fall into four
groups, not two:
1. **A*** attains the bound exactly (by construction — it is the forward contour).
2. **MM** is pinned at it, worst 0.976 / median 0.998, explained by the τ=C*/2 identity and
   verified on the worst case; the dips are boundary rounding, not looseness. A* and MM together
   are independent evidence the floor is not inflated on their groups.
3. **BiA* and BAE*** fall far below via a candidate/class escape (nipping + individual-f
   termination), so the bound does not bind them — expected, not a floor issue.
4. **NBS** falls to 0.377 (median 0.858) while in the class and not undercounting, against a floor
   A* corroborates on the same instances. This one is **unexplained** and reported as a limitation
   (297 of 46,886), with the failed mechanisms named and the termination hypothesis flagged.
No algorithm beats the optimum.

## Reproduce
```
./src/build.sh /tmp/vb            # driver emits `nipped`; needs hog2-patches/add_bae_nipped.py
# floor soundness on the worst BiA* case (out-of-class, expected below floor):
DUMP_INST=1886 /tmp/vb/mvc <plant02.3dmap> <plant02.3dscen> --start 1886 --limit 1
# MM = tau=C*/2 threshold cover (in-class, at the floor):
DUMP_INST=69 /tmp/vb/mvc <BSG.3dmap> <BSG.3dscen> --no-diagonals --limit 80   # -> /tmp/{fwd,bwd}G.txt
/tmp/vb/voxdriver <BSG.3dmap> <BSG.3dscen> --no-diagonals --limit 80 --algs mm
# compare MM_total to #{g_F<C*/2}+#{g_B<C*/2} from the dumped g-multisets.
# scan correctness: python3 cluster/verify_mvc_scan.py
```
