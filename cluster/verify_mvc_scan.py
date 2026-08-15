#!/usr/bin/env python3
"""
Verify that mvc.cpp's threshold-form MVC equals the true minimum vertex cover of the
must-expand graph (u~v iff g_F(u)+g_B(v) < C*). That graph is a chain graph, for which
threshold form = MVC by Konig; this checks the implementation against a direct maximum
bipartite matching (MVC = max matching) on random instances.

Settles whether a "sub-floor" score (exp/floor < 1) could be a floor over-computation:
if the scan matches the matching MVC, the floor is correctly computed and any sub-floor
score is the pairwise bound being loose for algorithms with tighter individual bounds
(MM's max(f,2g), BAE*'s f+d, NBS/BiA*'s individual f) -- NOT a bug.
"""
import random, bisect

def threshold_mvc(fwd, bwd, Cstar, eps=1e-7):          # transcribed from mvc.cpp
    fwd=sorted(fwd); bwd=sorted(bwd)
    def bwdCountLess(x): return bisect.bisect_left(bwd, x-eps)
    best=float('inf'); i=0
    while i<=len(fwd):
        bwdCost=0 if i==len(fwd) else bwdCountLess(Cstar-fwd[i])
        best=min(best, i+bwdCost)
        if i==len(fwd): break
        v=fwd[i]
        while i<len(fwd) and not (fwd[i]>v+eps): i+=1
    return best

def matching_mvc(fwd, bwd, Cstar):                      # Konig: MVC == max matching
    adj=[[j for j,gb in enumerate(bwd) if fwd[i]+gb < Cstar-1e-12] for i in range(len(fwd))]
    matchR=[-1]*len(bwd)
    def aug(u,seen):
        for v in adj[u]:
            if not seen[v]:
                seen[v]=True
                if matchR[v]==-1 or aug(matchR[v],seen):
                    matchR[v]=u; return True
        return False
    return sum(aug(u,[False]*len(bwd)) for u in range(len(fwd)))

if __name__=="__main__":
    random.seed(1); trials=3000; bad=0
    for _ in range(trials):
        fwd=[round(random.uniform(0,10),2) for _ in range(random.randint(1,30))]
        bwd=[round(random.uniform(0,10),2) for _ in range(random.randint(1,30))]
        Cstar=round(random.uniform(0,20),2)
        if threshold_mvc(fwd,bwd,Cstar)!=matching_mvc(fwd,bwd,Cstar): bad+=1
    print(f"{trials} random chain-graph trials: {bad} mismatches (0 => scan computes the true MVC)")
