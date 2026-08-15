#!/usr/bin/env python3
"""Convergence figure: running median exp/floor + bootstrap 95% CI band vs #instances,
for a few representative groups. Shows the median stabilising (the run-until-stable
evidence). Usage: plot_convergence.py <combined_long.csv> <out_dir>"""
import csv, sys, os, random, statistics as st
from collections import defaultdict
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt

COLOR={"astar":"#0072B2","bae":"#E69F00","bia":"#009E73","nbs":"#CC79A7"}
LABEL={"astar":"A*","bae":"BAE*","bia":"BiA*","nbs":"NBS"}
plt.rcParams.update({"figure.dpi":140,"savefig.dpi":200,"font.size":11,
    "axes.spines.top":False,"axes.spines.right":False,"axes.grid":True,
    "grid.color":"#e6e6e6","font.family":"DejaVu Sans"})

def boot_ci(vals,nb=300):
    if len(vals)<2: return (vals[0],vals[0]) if vals else (0,0)
    rng=random.Random(42)
    m=sorted(st.median(rng.choices(vals,k=len(vals))) for _ in range(nb))
    return m[int(0.025*len(m))], m[int(0.975*len(m))]

def main():
    path, out = sys.argv[1], sys.argv[2]
    series=defaultdict(list)  # (fam,cfg,alg) -> [exp/floor in run order]
    for r in csv.DictReader(open(path)):
        if r["status"]=="ok" and r["alg"] in COLOR:
            try:
                v=float(r["exp_over_mvc"]); series[(r["family"],r["config"],r["alg"])].append(v)
            except (ValueError,TypeError): pass
    # one representative converged group
    fam,cfg = "sandstone","diag"
    fig,ax=plt.subplots(figsize=(7.5,4.6))
    for alg in ("astar","bae","bia","nbs"):
        vals=series.get((fam,cfg,alg),[])
        if len(vals)<50: continue
        ks=list(range(25,len(vals)+1,max(1,len(vals)//60)))
        med=[st.median(vals[:k]) for k in ks]
        cis=[boot_ci(vals[:k]) for k in ks]
        lo=[c[0] for c in cis]; hi=[c[1] for c in cis]
        ax.plot(ks,med,color=COLOR[alg],label=LABEL[alg],lw=2)
        ax.fill_between(ks,lo,hi,color=COLOR[alg],alpha=0.15)
    ax.set_xlabel("number of instances (representative, shuffled order)")
    ax.set_ylabel("running median expansions / floor")
    ax.set_title("Convergence of the median with 95% bootstrap CI (sandstone, diagonal)")
    ax.legend(frameon=False,ncol=4)
    fig.tight_layout()
    os.makedirs(out,exist_ok=True)
    for ext in ("png","pdf"): fig.savefig(os.path.join(out,f"convergence.{ext}"),bbox_inches="tight")
    print("wrote convergence")

if __name__=="__main__": main()
