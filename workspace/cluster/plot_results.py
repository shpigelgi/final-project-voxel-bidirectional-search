#!/usr/bin/env python3
"""
Report figures from the aggregated benchmark data (matplotlib).

Usage:
    plot_results.py <results_dir> <out_dir> [--crossover <xover_results_dir>]

<results_dir> must contain aggregated/combined_long.csv (from aggregate.py).
Produces PNG + PDF figures in <out_dir>. Colours use the Okabe-Ito palette
(colourblind-safe by construction), assigned to algorithms in a FIXED order so
identity is stable across every figure.
"""
import csv, os, sys, statistics as st
from collections import defaultdict
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# Okabe-Ito (CVD-safe); fixed per-algorithm assignment. GBFS = grey (it is the
# non-optimal outlier, deliberately de-emphasised).
COLOR = {"astar":"#0072B2","rastar":"#56B4E9","mm":"#D55E00","bia":"#009E73",
         "bae":"#E69F00","nbs":"#CC79A7","gbfs":"#999999"}
LABEL = {"astar":"A*","rastar":"rev-A*","mm":"MM","bia":"BiA*","bae":"BAE*","nbs":"NBS","gbfs":"GBFS"}
OPT   = ["astar","rastar","mm","bia","bae","nbs"]           # optimal algorithms
ORDER = ["astar","bia","nbs","bae","mm","rastar"]           # display order (best->worst-ish)

plt.rcParams.update({
    "figure.dpi":140, "savefig.dpi":200, "font.size":11,
    "axes.spines.top":False, "axes.spines.right":False,
    "axes.grid":True, "grid.color":"#e6e6e6", "grid.linewidth":0.8,
    "axes.axisbelow":True, "font.family":"DejaVu Sans",
})

def load(results_dir):
    rows=list(csv.DictReader(open(os.path.join(results_dir,"aggregated","combined_long.csv"))))
    def f(x):
        try: return float(x)
        except: return None
    return rows, f

def fig_exp_over_mvc(rows, f, out):
    # median exp/MVC per (family,config,alg) for optimal algos
    vals=defaultdict(list)
    for r in rows:
        v=f(r["exp_over_mvc"])
        if v is not None and r["status"]=="ok" and r["alg"] in OPT:
            vals[(r["family"],r["config"],r["alg"])].append(v)
    groups=sorted({(k[0],k[1]) for k in vals})
    fig,ax=plt.subplots(figsize=(9,4.6))
    n=len(ORDER); w=0.8/n
    xs=range(len(groups))
    for i,alg in enumerate(ORDER):
        ys=[st.median(vals.get((fam,cfg,alg),[float("nan")])) if vals.get((fam,cfg,alg)) else 0 for fam,cfg in groups]
        bars=ax.bar([x+i*w for x in xs], ys, width=w, color=COLOR[alg], label=LABEL[alg], zorder=3)
    ax.axhline(1.0, color="#444", lw=1, ls="--", zorder=2)
    ax.text(len(groups)-0.5, 1.02, "must-expand floor (1.0)", ha="right", va="bottom", fontsize=9, color="#444")
    ax.set_xticks([x+0.4-w/2 for x in xs]); ax.set_xticklabels([f"{fam.split('-')[0]}\n{cfg}" for fam,cfg in groups])
    ax.set_ylabel("median expansions / must-expand floor")
    ax.set_title("Expansions relative to the theoretical minimum (lower = better)")
    ax.legend(ncol=6, fontsize=9, frameon=False, loc="upper center", bbox_to_anchor=(0.5,1.14))
    fig.tight_layout(); save(fig,out,"exp_over_mvc")

def fig_winrate(rows, f, out):
    inst=defaultdict(dict)
    for r in rows:
        if r["alg"] in OPT and r["status"]=="ok":
            e=f(r["expanded"])
            if e is not None: inst[(r["family"],r["config"],r["instance"])][r["alg"]]=e
    wins=defaultdict(int); tot=0
    for k,d in inst.items():
        if len(d)<len(OPT): continue
        tot+=1; wins[min(d,key=d.get)]+=1
    algs=[a for a in ORDER if wins.get(a,0)>0]
    ys=[100*wins[a]/tot for a in algs]
    fig,ax=plt.subplots(figsize=(7,4))
    ax.bar([LABEL[a] for a in algs],[y for y in ys],color=[COLOR[a] for a in algs],zorder=3)
    for i,y in enumerate(ys): ax.text(i,y+0.6,f"{y:.0f}%",ha="center",fontsize=10)
    ax.set_ylabel("% of instances with fewest expansions")
    ax.set_title(f"Per-instance winner among optimal algorithms (n={tot})")
    fig.tight_layout(); save(fig,out,"winrate")

def fig_asymmetry(rows, f, out):
    inst=defaultdict(dict)
    for r in rows:
        if r["alg"] in ("astar","rastar") and r["status"]=="ok":
            e=f(r["expanded"])
            if e is not None: inst[(r["family"],r["config"],r["instance"])][r["alg"]]=e
    groups=sorted({(k[0],k[1]) for k in inst})
    ratios=[]
    for fam,cfg in groups:
        rr=[inst[k]["rastar"]/inst[k]["astar"] for k in inst if (k[0],k[1])==(fam,cfg)
            and "rastar" in inst[k] and inst[k].get("astar")]
        ratios.append(st.median(rr) if rr else 0)
    fig,ax=plt.subplots(figsize=(7,4))
    ax.bar([f"{fam.split('-')[0]}\n{cfg}" for fam,cfg in groups],ratios,color="#56B4E9",zorder=3)
    ax.axhline(1.0,color="#444",lw=1,ls="--");
    for i,v in enumerate(ratios): ax.text(i,v+0.03,f"{v:.2f}x",ha="center",fontsize=10)
    ax.set_ylabel("median  reverse-A* / forward-A*  expansions")
    ax.set_title("Directional asymmetry (>1 = searching from the goal is harder)")
    fig.tight_layout(); save(fig,out,"directional_asymmetry")

def fig_crossover(xdir, f, out):
    rows,_=load(xdir)
    # config is like 'w100','w050'... -> weight
    series=defaultdict(dict)  # alg -> {weight: median expansions over ok instances}
    byw=defaultdict(lambda: defaultdict(list))
    for r in rows:
        if r["alg"] in OPT and r["status"]=="ok":
            e=f(r["expanded"])
            if e is None: continue
            w=int(r["config"][1:])/100.0
            byw[r["alg"]][w].append(e)
    fig,ax=plt.subplots(figsize=(7.5,4.8))
    for alg in ORDER:
        ws=sorted(byw[alg]);
        if not ws: continue
        ys=[st.median(byw[alg][w]) for w in ws]
        ax.plot([w for w in ws],ys,marker="o",color=COLOR[alg],label=LABEL[alg],lw=2,ms=6,zorder=3)
    ax.set_yscale("log"); ax.invert_xaxis()
    ax.set_xlabel("heuristic weight w  (1.0 = full octile → 0 = Dijkstra; weaker to the right)")
    ax.set_ylabel("median expansions (log)")
    ax.set_title("Heuristic-strength crossover: expansions vs. heuristic weight")
    ax.legend(ncol=3,fontsize=9,frameon=False)
    fig.tight_layout(); save(fig,out,"crossover")

def save(fig,out,name):
    os.makedirs(out,exist_ok=True)
    for ext in ("png","pdf"): fig.savefig(os.path.join(out,f"{name}.{ext}"),bbox_inches="tight")
    plt.close(fig); print("wrote",name)

if __name__=="__main__":
    results=sys.argv[1]; out=sys.argv[2]
    rows,f=load(results)
    fig_exp_over_mvc(rows,f,out); fig_winrate(rows,f,out); fig_asymmetry(rows,f,out)
    if "--crossover" in sys.argv:
        xdir=sys.argv[sys.argv.index("--crossover")+1]
        try: fig_crossover(xdir,f,out)
        except Exception as e: print("crossover fig skipped:",e)
