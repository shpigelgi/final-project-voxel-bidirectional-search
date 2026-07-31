#!/usr/bin/env python3
"""
Aggregate cluster results into tidy + summary CSVs.

Usage:
    aggregate.py <results_dir> [out_dir]

<results_dir> holds per-map files written by run_array.sbatch:
    <family>/<base>.<config>.exp.csv    tag,instance,alg,expanded,generated,cost,optimal,time_ms,status
    <family>/<base>.<config>.mvc.csv    instance,cstar,fwd_cand,bwd_cand,mvc

Outputs (default out_dir = <results_dir>/aggregated):
    combined_long.csv   one row per (family,map,config,instance,alg) with the MVC join
    summary.csv         per (family,config,alg): counts, %status, mean expanded,
                        mean/median expanded-over-MVC, mean time
It only depends on the standard library.
"""
import csv, glob, os, sys, statistics
from collections import defaultdict

def load_mvc(path):
    m = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            try:
                m[int(row["instance"])] = float(row["mvc"])
            except (ValueError, KeyError):
                pass
    return m

def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    results_dir = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else os.path.join(results_dir, "aggregated")
    os.makedirs(out_dir, exist_ok=True)

    long_rows = []
    for exp_path in sorted(glob.glob(os.path.join(results_dir, "*", "*.exp.csv"))):
        family = os.path.basename(os.path.dirname(exp_path))
        fname = os.path.basename(exp_path)[:-len(".exp.csv")]  # "<base>.<config>"
        base, _, config = fname.rpartition(".")
        mvc_path = exp_path[:-len(".exp.csv")] + ".mvc.csv"
        mvc = load_mvc(mvc_path) if os.path.exists(mvc_path) else {}
        with open(exp_path) as f:
            for row in csv.DictReader(f):
                try:
                    inst = int(row["instance"])
                    exp = int(row["expanded"])
                except (ValueError, KeyError):
                    continue
                floor = mvc.get(inst)
                ratio = (exp / floor) if (floor and floor > 0) else ""
                long_rows.append({
                    "family": family, "map": base, "config": config, "instance": inst,
                    "alg": row["alg"], "expanded": exp,
                    "cost": row.get("cost", ""), "optimal": row.get("optimal", ""),
                    "time_ms": row.get("time_ms", ""), "status": row.get("status", ""),
                    "peak_mb": row.get("peak_mb", ""),
                    "mvc": floor if floor is not None else "", "exp_over_mvc": ratio,
                })

    long_csv = os.path.join(out_dir, "combined_long.csv")
    fields = ["family","map","config","instance","alg","expanded","cost","optimal",
              "time_ms","status","peak_mb","mvc","exp_over_mvc"]
    with open(long_csv, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields); w.writeheader(); w.writerows(long_rows)

    # ---- summary per (family, config, alg) ----
    groups = defaultdict(list)
    for r in long_rows:
        groups[(r["family"], r["config"], r["alg"])].append(r)

    summ = []
    for (family, config, alg), rows in sorted(groups.items()):
        n = len(rows)
        st = defaultdict(int)
        for r in rows: st[r["status"]] += 1
        exps = [r["expanded"] for r in rows if r["status"] == "ok"]
        ratios = [r["exp_over_mvc"] for r in rows
                  if r["status"] == "ok" and isinstance(r["exp_over_mvc"], float)]
        times = [float(r["time_ms"]) for r in rows if r["status"] == "ok" and r["time_ms"]]
        peaks = [float(r["peak_mb"]) for r in rows if r["status"] == "ok" and r["peak_mb"]]
        summ.append({
            "family": family, "config": config, "alg": alg, "n": n,
            "pct_ok": round(100*st.get("ok",0)/n, 1) if n else 0,
            "pct_subopt": round(100*st.get("subopt",0)/n, 1) if n else 0,
            "pct_timeout": round(100*st.get("timeout",0)/n, 1) if n else 0,
            "pct_fail": round(100*(st.get("oom",0)+st.get("error",0)+st.get("nopath",0))/n, 1) if n else 0,
            "mean_expanded": round(statistics.mean(exps), 1) if exps else "",
            "mean_exp_over_mvc": round(statistics.mean(ratios), 2) if ratios else "",
            "median_exp_over_mvc": round(statistics.median(ratios), 2) if ratios else "",
            "mean_time_ms": round(statistics.mean(times), 3) if times else "",
            "median_peak_mb": round(statistics.median(peaks), 1) if peaks else "",
        })

    summ_csv = os.path.join(out_dir, "summary.csv")
    sfields = ["family","config","alg","n","pct_ok","pct_subopt","pct_timeout","pct_fail",
               "mean_expanded","mean_exp_over_mvc","median_exp_over_mvc","mean_time_ms",
               "median_peak_mb"]
    with open(summ_csv, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=sfields); w.writeheader(); w.writerows(summ)

    # ---- cross-algorithm optimality consistency (per instance the optimal algos must agree) ----
    # This is the optimality check for BOTH modes; in nodiag it is the ONLY check, since the
    # scenario's optimal cost does not apply there.
    OPT_ALGS = {"astar", "rastar", "mm", "bae", "nbs"}
    per_inst = defaultdict(dict)  # (family,map,config,instance) -> {alg: cost}
    for r in long_rows:
        if r["alg"] in OPT_ALGS and r["status"] == "ok":
            try: per_inst[(r["family"], r["map"], r["config"], r["instance"])][r["alg"]] = float(r["cost"])
            except ValueError: pass
    disagree = defaultdict(int); checked = defaultdict(int)
    for (family, mp, config, inst), costs in per_inst.items():
        if len(costs) < 2: continue
        checked[(family, config)] += 1
        if max(costs.values()) - min(costs.values()) > 1e-2:
            disagree[(family, config)] += 1

    print(f"Wrote {long_csv} ({len(long_rows)} rows)")
    print(f"Wrote {summ_csv} ({len(summ)} groups)")
    print("\nOptimality consistency (optimal algos agree per instance):")
    for key in sorted(checked):
        fam, cfg = key
        print(f"  {fam}/{cfg}: {checked[key]-disagree[key]}/{checked[key]} consistent"
              + (f"  ** {disagree[key]} DISAGREEMENTS **" if disagree[key] else ""))
    # Console preview
    print("\nfamily/config/alg            n   %ok  %to  mean_exp   exp/MVC(med)")
    for s in summ:
        print(f"{s['family'][:12]:12} {s['config']:6} {s['alg']:6} {s['n']:5} "
              f"{s['pct_ok']:5} {s['pct_timeout']:4} {str(s['mean_expanded']):>9}  "
              f"{str(s['median_exp_over_mvc']):>6}")

if __name__ == "__main__":
    main()
