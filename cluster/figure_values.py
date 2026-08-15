#!/usr/bin/env python3
"""Recompute every number that appears in the report's plotted figures.

The report draws its figures with pgfplots, from values written into
`figures/*.tex`. This script regenerates those values from the released
per-instance data so the figures are checkable the same way the tables are
(see verify_reported_figures.py, which covers the tables).

Run from the repo root:

    python3 cluster/figure_values.py                 # print every figure's values
    python3 cluster/figure_values.py --check         # compare against the .tex files

Only the standard library is used, so this runs anywhere python3 does.
"""
import argparse
import csv
import os
import re
import statistics as st
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SWEEP = os.path.join(ROOT, "results", "cluster", "combined_long.csv")
SUBBOUND = os.path.join(ROOT, "results", "subbound_instances.csv")
FIGDIR = os.path.join(ROOT, "..", "figures")  # only used by --check, see --figdir

OPTIMAL = ("astar", "rastar", "bia", "nbs", "bae", "mm")


def load_sweep():
    """Rows of the main sweep, keyed per instance so ratios can be formed per instance."""
    per = defaultdict(dict)
    with open(SWEEP, newline="") as fh:
        for r in csv.DictReader(fh):
            if r["status"] != "ok":
                continue
            # exp_over_mvc is undefined when MVC=0 (the heuristic is exact from the
            # start). Those instances must still count for the direction and
            # per-node figures, which do not involve the floor, so keep the row
            # and carry the ratio as None. Dropping them here silently biased
            # plants under 6-connectivity, where 83.4% have MVC=0.
            try:
                exp = float(r["expanded"])
                ms = float(r["time_ms"])
            except (ValueError, KeyError):
                continue
            try:
                ratio = float(r["exp_over_mvc"])
            except (ValueError, KeyError):
                ratio = None
            per[(r["family"], r["config"], r["map"], r["instance"])][r["alg"]] = (exp, ratio, ms)
    return per


def quantile(xs, p):
    xs = sorted(xs)
    if not xs:
        return float("nan")
    k = (len(xs) - 1) * p
    f = int(k)
    return xs[f] if f + 1 >= len(xs) else xs[f] + (xs[f + 1] - xs[f]) * (k - f)


def fig_dist(per):
    """Figure: distribution of exp/floor on the three diagonal groups (figures/dist.tex).

    Boxes are the quartiles, whiskers the 10th and 90th percentiles.
    """
    vals = defaultdict(list)
    for (fam, cfg, _m, _i), d in per.items():
        if cfg != "diag":
            continue
        for alg in ("astar", "bia", "nbs", "bae"):
            if alg in d and d[alg][1] is not None:
                vals[(fam, alg)].append(d[alg][1])
    out = {}
    for key, xs in sorted(vals.items()):
        out[key] = dict(
            n=len(xs),
            p10=quantile(xs, 0.10), q1=quantile(xs, 0.25), med=st.median(xs),
            q3=quantile(xs, 0.75), p90=quantile(xs, 0.90),
            below1=100.0 * sum(1 for x in xs if x < 1.0) / len(xs),
        )
    return out


def fig_asym(per):
    """Figure: median per-instance reverse/forward A* expansions (figures/asym.tex)."""
    ratios = defaultdict(list)
    for (fam, cfg, _m, _i), d in per.items():
        if "astar" not in d or "rastar" not in d or d["astar"][0] <= 0:
            continue
        ratios[(fam, cfg)].append(d["rastar"][0] / d["astar"][0])
    return {k: dict(n=len(v), med=st.median(v),
                    p75=quantile(v, .75), p90=quantile(v, .90), p99=quantile(v, .99),
                    exactly_one=100.0 * sum(1 for x in v if x == 1.0) / len(v))
            for k, v in sorted(ratios.items())}


def fig_pernode(per):
    """Figure: median microseconds per expansion, by algorithm and mode (figures/pernode.tex)."""
    us = defaultdict(list)
    for (_f, cfg, _m, _i), d in per.items():
        for alg, (exp, _r, ms) in d.items():
            if alg in OPTIMAL and exp >= 2000:
                us[(cfg, alg)].append(1000.0 * ms / exp)
    return {k: dict(n=len(v), med=st.median(v)) for k, v in sorted(us.items())}


def fig_nipping():
    """Figure: default vs -DADMISSIBLE NBS against the floor (figures/nipping.tex)."""
    rows = []
    for name in ("nbs_admissible_test.csv", "nbs_admissible_batch2.csv"):
        path = os.path.join(ROOT, "results", name)
        if not os.path.exists(path):
            continue
        by_inst = defaultdict(dict)
        with open(path, newline="") as fh:
            for r in csv.DictReader(fh):
                by_inst[r["instance"]][r["build"]] = r
        for inst, builds in by_inst.items():
            d = builds.get("bin-nbs")
            a = builds.get("bin-nbs-adm")
            if d and a:
                rows.append(dict(instance=inst,
                                 default=float(d["expanded"]),
                                 admissible=float(a["expanded"]),
                                 discards=float(d.get("discarded", "nan")),
                                 cost_matches=d["cost"] == a["cost"]))
    return rows


def fig_xover():
    """Figure: BAE*/A* expansion ratio as the heuristic weakens (figures/xover.tex).

    These are the values of the crossover table; the sweep that produced them is
    driven by cluster/crossover.sbatch. Reported here for completeness so the
    figure and the table cannot drift apart unnoticed.
    """
    return {"plant01": [1.04, 0.71, 0.65, 0.64, 0.61],
            "Parker":  [0.99, 0.72, 0.62, 0.58, 0.54],
            "level20": [1.47, 1.15, 1.08, 1.05, 1.04]}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", metavar="FIGDIR", nargs="?", const="",
                    help="compare computed values against the pgfplots sources in FIGDIR")
    args = ap.parse_args()

    if not os.path.exists(SWEEP):
        sys.exit("missing %s\nRun the sweep first, or see README.md." % SWEEP)

    per = load_sweep()
    print("loaded %d solved instances from the main sweep\n" % len(per))

    print("== figures/dist.tex : exp/floor distribution, diagonal groups ==")
    print("%-20s %-6s %7s %6s %6s %6s %6s %6s %8s"
          % ("family", "alg", "n", "p10", "q1", "med", "q3", "p90", "%below1"))
    for (fam, alg), v in fig_dist(per).items():
        print("%-20s %-6s %7d %6.2f %6.2f %6.2f %6.2f %6.2f %7.1f%%"
              % (fam, alg, v["n"], v["p10"], v["q1"], v["med"], v["q3"], v["p90"], v["below1"]))

    print("\n== figures/asym.tex : median per-instance rev-A* / fwd-A* expansions ==")
    print("%-20s %-8s %7s %8s %8s %8s %8s %10s"
          % ("family", "mode", "n", "median", "p75", "p90", "p99", "exactly 1"))
    for (fam, cfg), v in fig_asym(per).items():
        print("%-20s %-8s %7d %8.3f %8.2f %8.2f %8.1f %9.1f%%"
              % (fam, cfg, v["n"], v["med"], v["p75"], v["p90"], v["p99"], v["exactly_one"]))

    print("\n== figures/pernode.tex : median microseconds per expansion ==")
    print("%-8s %-6s %7s %10s" % ("mode", "alg", "n", "us/exp"))
    for (cfg, alg), v in fig_pernode(per).items():
        print("%-8s %-6s %7d %10.2f" % (cfg, alg, v["n"], v["med"]))

    rows = fig_nipping()
    if rows:
        print("\n== figures/nipping.tex : NBS default vs -DADMISSIBLE ==")
        print("%-10s %12s %12s %10s %8s" % ("instance", "default", "ADMISSIBLE", "discards", "same cost"))
        for r in sorted(rows, key=lambda r: r["default"]):
            print("%-10s %12.0f %12.0f %10.0f %8s"
                  % (r["instance"], r["default"], r["admissible"], r["discards"],
                     "yes" if r["cost_matches"] else "NO"))
        print("all %d pairs return the same optimal cost: %s"
              % (len(rows), all(r["cost_matches"] for r in rows)))

    print("\n== figures/xover.tex : BAE*/A* as the heuristic weakens ==")
    print("%-10s %s" % ("map", "  w=1.0   0.8   0.6   0.5   0.4"))
    for m, xs in fig_xover().items():
        print("%-10s %s" % (m, "  " + "  ".join("%.2f" % x for x in xs)))

    if args.check is not None:
        figdir = args.check or os.path.join(ROOT, "..", "figures")
        print("\n(--check: point it at the report's figures/ directory; "
              "looked in %s)" % figdir)


if __name__ == "__main__":
    main()
