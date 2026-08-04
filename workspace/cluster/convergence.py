#!/usr/bin/env python3
"""
Bootstrap-median convergence check for the "run until stable" sweep.

For each (family, config, alg) it computes the median exp/MVC over solved
instances and a bootstrap 95% CI, and reports the relative CI half-width. A
group is "converged" when that half-width is within the target (default 3%).

Also prints, per (family, config), the max instance index seen so far and
whether every algorithm has converged - i.e. whether that group still needs
more instances (extend) or can stop.

    convergence.py <combined_long.csv> [target_pct] [n_boot]
"""
import sys, csv, random, statistics as st
from collections import defaultdict

def boot_ci(vals, n_boot, lo=2.5, hi=97.5):
    if len(vals) < 2:
        return (vals[0], vals[0]) if vals else (0.0, 0.0)
    rng = random.Random(42)
    meds = sorted(st.median(rng.choices(vals, k=len(vals))) for _ in range(n_boot))
    def pct(p):
        i = min(len(meds)-1, max(0, int(round(p/100*(len(meds)-1)))))
        return meds[i]
    return pct(lo), pct(hi)

def main():
    path = sys.argv[1]
    target = float(sys.argv[2]) if len(sys.argv) > 2 else 3.0
    n_boot = int(sys.argv[3]) if len(sys.argv) > 3 else 2000
    OPT = {"astar","rastar","mm","bia","bae","nbs"}

    ratios = defaultdict(list)      # (family,config,alg) -> [exp/mvc over ok]
    maxidx = defaultdict(int)       # (family,config) -> max instance index
    for r in csv.DictReader(open(path)):
        fam, cfg, alg = r["family"], r["config"], r["alg"]
        try: maxidx[(fam,cfg)] = max(maxidx[(fam,cfg)], int(r["instance"]))
        except: pass
        if alg in OPT and r["status"] == "ok":
            try: ratios[(fam,cfg,alg)].append(float(r["exp_over_mvc"]))
            except: pass

    groups = defaultdict(dict)      # (fam,cfg) -> {alg: (median, rel_ci%, n, converged)}
    for (fam,cfg,alg), vals in sorted(ratios.items()):
        if not vals: continue
        med = st.median(vals)
        lo, hi = boot_ci(vals, n_boot)
        rel = 100*(hi-lo)/2/med if med > 0 else 0.0
        groups[(fam,cfg)][alg] = (med, rel, len(vals), rel <= target)

    print(f"{'family':18}{'cfg':7}{'alg':7}{'n_ok':>6}{'median':>9}{'relCI%':>8}  conv")
    need = []
    for (fam,cfg) in sorted(groups):
        allconv = True
        for alg in ("astar","bia","nbs","bae","mm","rastar"):
            if alg not in groups[(fam,cfg)]: continue
            med, rel, n, conv = groups[(fam,cfg)][alg]
            allconv = allconv and conv
            print(f"{fam:18}{cfg:7}{alg:7}{n:>6}{med:>9.2f}{rel:>8.1f}  {'yes' if conv else 'NO'}")
        status = "CONVERGED" if allconv else "extend"
        print(f"  -> ({fam},{cfg}) maxidx={maxidx[(fam,cfg)]}  {status}")
        if not allconv and maxidx[(fam,cfg)] < 1999:
            need.append(f"{fam}:{cfg}")
    print("\nNEED_EXTEND=" + ",".join(need) if need else "\nNEED_EXTEND=  (all converged or capped)")

if __name__ == "__main__":
    main()
