#!/usr/bin/env python3
"""Check every figure printed in the paper against the committed artifacts.

Run from the repository root:

    python3 results/verify_reported_figures.py

Exits 0 if every figure reproduces, 1 otherwise. Each check names the paper
location it covers, so a failure tells you which sentence or cell to fix.

Conventions the paper uses, which this script mirrors:

  * exp/floor is formed per instance and then medianed. It is defined only on
    non-trivial instances (mvc >= 1) and only over solved runs (status == ok).
  * Table 2 is restricted to instances that all four listed algorithms solved,
    so every column covers the same set. This matters: under a per-column
    definition the n values inflate by up to 1.8% and one cell moves.
  * Table 5 and the main-sweep BAE*/A* figures are medians of per-instance
    ratios, matching the rest of the paper.
  * Integer cells are round-half-up. Python's round() is banker's rounding, so
    a median of exactly 378.5 prints as 379 and round() would give 378.
"""

import csv
import collections
import glob
import os
import statistics as st
import sys

# Paths are resolved relative to this file, so the script runs from anywhere.
_HERE = os.path.dirname(os.path.abspath(__file__))
CLUSTER = os.path.join(_HERE, "cluster", "combined_long.csv")
CROSSOVER = os.path.join(_HERE, "crossover", "representative")

SHORT = {"industrial-plants": "plants", "sandstone": "sandstone", "descent": "descent"}
ALGS6 = ["astar", "bia", "nbs", "bae", "mm", "rastar"]
FOUR = ["astar", "bia", "nbs", "bae"]
PER_MAP = {"industrial-plants": 1500, "sandstone": 500, "descent": 500}

results = []


def check(label, printed, computed):
    results.append((label, printed, computed, printed == computed))


def num(x):
    try:
        return float(x)
    except (TypeError, ValueError):
        return None


def half_up(x):
    return int(x + 0.5)


def load(path):
    if not os.path.exists(path):
        sys.exit(f"missing artifact: {path}\nrun from the repository root")
    return list(csv.DictReader(open(path)))


def main():
    rows = load(CLUSTER)
    solved = [r for r in rows if r["status"] == "ok"]

    # ---- run accounting, axis B ----
    status = collections.Counter(r["status"] for r in rows)
    check("accounting: total records", 384223, len(rows))
    check("accounting: solved", 302358, status["ok"])
    check("accounting: timed out", 27015, status["timeout"])
    check("accounting: GBFS suboptimal", 54850, status["subopt"])
    check("accounting: identity closes", len(rows),
          status["ok"] + status["timeout"] + status["subopt"])
    dup = collections.Counter(
        (r["config"], r["family"], r["map"], r["instance"], r["alg"]) for r in rows)
    check("accounting: no duplicate runs", 0, sum(v - 1 for v in dup.values()))
    designed = sum(PER_MAP[f] * len({r["map"] for r in rows if r["family"] == f})
                   for f in PER_MAP)
    check("accounting: instances designed per mode", 27500, designed)
    check("accounting: 385,000 planned minus 742 minus 35", 384223,
          designed * 2 * 7 - 106 * 7 - 35)

    # ---- Table 1, exp/floor ----
    t1 = {
        ("plants", "diag"): [1.00, 1.62, 1.69, 1.57, 3.39, 1.73],
        ("plants", "nodiag"): [1.06, 2.03, 3.41, 3.62, 4.60, 3.04],
        ("sandstone", "diag"): [1.00, 1.54, 1.66, 0.95, 2.63, 1.35],
        ("sandstone", "nodiag"): [1.00, 1.73, 1.56, 1.25, 1.63, 1.58],
        ("descent", "diag"): [1.00, 1.63, 1.58, 1.47, 2.74, 1.38],
        ("descent", "nodiag"): [1.00, 1.78, 1.72, 1.72, 1.84, 1.47],
    }
    ratios = collections.defaultdict(list)
    for r in solved:
        m, q = num(r["mvc"]), num(r["exp_over_mvc"])
        if m is not None and m >= 1 and q is not None:
            ratios[(SHORT[r["family"]], r["config"], r["alg"])].append(q)
    for (fam, cfg), printed in t1.items():
        for alg, want in zip(ALGS6, printed):
            got = round(st.median(ratios[(fam, cfg, alg)]), 2)
            check(f"Table 1 {fam}.{cfg} {alg}", want, got)

    # ---- Table 2, stratified, over instances all four solved ----
    per_inst = collections.defaultdict(dict)
    for r in solved:
        m, q = num(r["mvc"]), num(r["exp_over_mvc"])
        if m is not None and m >= 1 and q is not None:
            per_inst[(r["config"], r["family"], r["map"], r["instance"])][r["alg"]] = q
    t2 = [("[1.00,1.01)", 1.00, 1.01, 36042, [1.00, 1.64, 1.65, 1.44]),
          ("[1.01,1.10)", 1.01, 1.10, 5140, [1.03, 1.75, 1.62, 1.51]),
          ("[1.10,2.0)", 1.10, 2.00, 4059, [1.24, 1.83, 1.53, 1.52]),
          ("[2.0,inf)", 2.00, float("inf"), 1311, [6.13, 3.53, 12.40, 7.06])]
    for label, lo, hi, want_n, want in t2:
        keys = [k for k, d in per_inst.items()
                if all(a in d for a in FOUR) and lo <= d["astar"] < hi]
        check(f"Table 2 {label} n", want_n, len(keys))
        for alg, w in zip(FOUR, want):
            check(f"Table 2 {label} {alg}", w,
                  round(st.median(per_inst[k][alg] for k in keys), 2))

    # ---- Table 3, memory ----
    t3 = {
        ("plants", "diag"): [97, 110, 107, 104, 241, 184],
        ("plants", "nodiag"): [11, 11, 104, 11, 42, 14],
        ("sandstone", "diag"): [181, 208, 193, 152, 28, 207],
        ("sandstone", "nodiag"): [101, 143, 127, 97, 148, 181],
        ("descent", "diag"): [368, 411, 364, 407, 379, 430],
        ("descent", "nodiag"): [271, 364, 319, 359, 411, 403],
    }
    peaks = collections.defaultdict(list)
    for r in solved:
        v = num(r["peak_mb"])
        if v is not None:
            peaks[(SHORT[r["family"]], r["config"], r["alg"])].append(v)
    for (fam, cfg), printed in t3.items():
        for alg, want in zip(ALGS6, printed):
            check(f"Table 3 {fam}.{cfg} {alg}", want,
                  half_up(st.median(peaks[(fam, cfg, alg)])))
    check("Axis C: no memory-exhaustion failures", True,
          all(r["status"] in ("ok", "subopt", "timeout") for r in rows))

    # ---- axis C, timeout rates ----
    tot = collections.Counter(r["alg"] for r in rows)
    outs = collections.Counter(r["alg"] for r in rows if r["status"] == "timeout")
    for alg, want in (("bae", 1.28), ("nbs", 1.21), ("bia", 0.91),
                      ("rastar", 0.60), ("astar", 0.07)):
        check(f"Axis C sweep timeout rate {alg}", want,
              round(100 * outs[alg] / tot[alg], 2))
    check("Axis C sweep timeout rate mm", 45.1, round(100 * outs["mm"] / tot["mm"], 1))
    for fam, want in (("industrial-plants", 77), ("descent", 94), ("sandstone", 98)):
        grp = [r for r in rows if r["alg"] == "mm" and r["config"] == "diag"
               and r["family"] == fam]
        check(f"Axis C MM diagonal timeout {SHORT[fam]}", want,
              half_up(100 * sum(1 for r in grp if r["status"] == "timeout") / len(grp)))
    for alg, want in (("bae", 4.6), ("bia", 3.2), ("nbs", 3.1), ("astar", 0.3)):
        grp = [r for r in rows if r["alg"] == alg and r["config"] == "diag"
               and r["family"] == "descent"]
        check(f"Axis C descent.diag timeout {alg}", want,
              round(100 * sum(1 for r in grp if r["status"] == "timeout") / len(grp), 1))

    # ---- axis A, BAE* sub-floor shares quoted in the text and abstract ----
    shares = []
    for fam in PER_MAP:
        for cfg in ("diag", "nodiag"):
            v = ratios[(SHORT[fam], cfg, "bae")]
            shares.append(100 * sum(1 for x in v if x < 1) / len(v))
    check("Axis A BAE* sub-floor share, low", 0.6, round(min(shares), 1))
    check("Axis A BAE* sub-floor share, high", 55.6, round(max(shares), 1))

    # ---- axis B, per-expansion cost on diagonal maps ----
    def micros(r):
        e = num(r["expanded"])
        return (num(r["time_ms"]) * 1000.0 / e) if e else None

    def diag_range(alg):
        out = []
        for fam in PER_MAP:
            v = [micros(r) for r in solved
                 if r["alg"] == alg and r["config"] == "diag" and r["family"] == fam]
            v = [x for x in v if x is not None]
            if v:
                out.append(st.median(v))
        return round(min(out), 1), round(max(out), 1)

    check("Axis B per-expansion A*", (1.9, 2.7), diag_range("astar"))
    check("Axis B per-expansion BAE*", (3.7, 4.6), diag_range("bae"))
    check("Axis B per-expansion MM", (8.4, 19.6), diag_range("mm"))
    joint = diag_range("bia") + diag_range("nbs")
    check("Axis B per-expansion BiA*+NBS joint", (2.8, 3.9),
          (min(joint), max(joint)))
    pooled = [micros(r) for r in solved if r["alg"] == "mm"]
    check("Axis B MM pooled per-expansion", 1.4,
          round(st.median([x for x in pooled if x is not None]), 1))

    # ---- Table 5 and the main-sweep ranges, median of per-instance ratios ----
    if os.path.isdir(CROSSOVER):
        xs = collections.defaultdict(lambda: collections.defaultdict(dict))
        for path in glob.glob(os.path.join(CROSSOVER, "*.exp.csv")):
            mp, w = os.path.basename(path).split(".")[:2]
            for r in csv.DictReader(open(path)):
                if r["status"] == "ok":
                    xs[(mp, w)][r["alg"]][r["instance"]] = float(r["expanded"])
        t5 = {"plant01": [1.04, 0.71, 0.65, 0.64, 0.61],
              "Parker": [0.99, 0.72, 0.62, 0.58, 0.54],
              "level20": [1.47, 1.15, 1.08, 1.05, 1.04]}
        for mp, printed in t5.items():
            for w, want in zip(["w100", "w080", "w060", "w050", "w040"], printed):
                g = xs[(mp, w)]
                ks = [k for k in g["bae"] if k in g["astar"] and g["astar"][k] > 0]
                check(f"Table 5 {mp} {w}", want,
                      round(st.median(g["bae"][k] / g["astar"][k] for k in ks), 2))
    else:
        print(f"note: {CROSSOVER} absent, skipping Table 5")

    exp = collections.defaultdict(dict)
    fam_of = {}
    for r in solved:
        if r["config"] == "diag":
            exp[r["alg"]][(r["map"], r["instance"])] = float(r["expanded"])
            fam_of[r["map"]] = r["family"]
    by_fam = collections.defaultdict(list)
    plant01 = None
    for mp in {k[0] for k in exp["astar"]}:
        ks = [k for k in exp["bae"]
              if k in exp["astar"] and k[0] == mp and exp["astar"][k] > 0]
        if len(ks) < 50:
            continue
        v = st.median(exp["bae"][k] / exp["astar"][k] for k in ks)
        by_fam[fam_of[mp]].append(v)
        if mp == "plant01":
            plant01 = v
    check("main sweep plants range", (1.03, 2.28),
          (round(min(by_fam["industrial-plants"]), 2),
           round(max(by_fam["industrial-plants"]), 2)))
    check("main sweep descent range", (0.90, 2.12),
          (round(min(by_fam["descent"]), 2), round(max(by_fam["descent"]), 2)))
    check("main sweep plant01", 1.14, round(plant01, 2))

    # ---- report ----
    failed = [x for x in results if not x[3]]
    for label, printed, computed, ok in results:
        if not ok:
            print(f"FAIL  {label}: paper {printed}, artifacts {computed}")
    print(f"\n{len(results) - len(failed)}/{len(results)} figures reproduce")
    if failed:
        print("the paper and the artifacts disagree; fix one of them")
        return 1
    print("every figure printed in the paper reproduces from the committed artifacts")
    return 0


if __name__ == "__main__":
    sys.exit(main())
