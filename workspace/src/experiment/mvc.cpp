//
//  mvc.cpp
//  Per-instance "must-expand floor": the Minimum Vertex Cover of the
//  must-expand graph G_MX (Eckerle 2017 / Chen 2017), i.e. the provable
//  minimum number of node expansions any admissible front-to-end bidirectional
//  algorithm needs. This is the denominator for expansion-ratio analysis.
//
//  Method (Shaham et al. 2017, base admissible case):
//    A forward node u is a candidate iff f_F(u) = g_F(u)+h_F(u) < C*.
//    A backward node v is a candidate iff f_B(v) = g_B(v)+h_B(v) < C*.
//    (u,v) is a must-expand edge iff g_F(u)+g_B(v) < C*.
//    For ANY threshold tau, {u: g_F(u)<tau} U {v: g_B(v)<C*-tau} is a valid
//    vertex cover, and the MVC equals the minimum of its size over tau
//    (the optimal cover has this threshold form). So:
//        |VC| = min_tau ( #{g_F < tau} + #{g_B < C*-tau} ).
//
//  We obtain the g-value multisets by expanding each direction's full f<C*
//  contour (a consistent heuristic makes the first settle of a node optimal).
//
//  Usage: mvc <map.3dmap> <scen.3dscen> [--limit N] [--no-diagonals]
//  Output CSV: instance,cstar,fwd_cand,bwd_cand,mvc
//
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include "../core/VoxelMap.h"
#include "TemplateAStar.h"
#include "FPUtil.h"

struct QN {
	double f, g;
	voxState s;
};
struct QNCmp { bool operator()(const QN &a, const QN &b) const { return a.f > b.f; } };

// Expand the f < Cstar contour from `origin` toward `toward` (heuristic aims at
// `toward`). Record the optimal g-value of every settled node with f < Cstar.
static void Contour(VoxelMap &env, const voxState &origin, const voxState &toward,
					double Cstar, std::vector<double> &gValues)
{
	std::priority_queue<QN, std::vector<QN>, QNCmp> open;
	std::unordered_map<uint64_t, double> closed;
	std::vector<voxState> succ;

	QN startN{env.HCost(origin, toward), 0.0, origin};
	open.push(startN);

	while (!open.empty())
	{
		QN cur = open.top(); open.pop();
		uint64_t h = env.GetStateHash(cur.s);
		auto it = closed.find(h);
		if (it != closed.end() && flesseq(it->second, cur.g))
			continue; // already settled with <= g
		closed[h] = cur.g;

		if (!fless(cur.f, Cstar))
			continue; // f >= C*: not a must-expand candidate; also nothing cheaper beyond
		gValues.push_back(cur.g);

		env.GetSuccessors(cur.s, succ);
		for (const auto &n : succ)
		{
			double g2 = cur.g + env.GCost(cur.s, n);
			double f2 = g2 + env.HCost(n, toward);
			if (!fless(f2, Cstar)) continue;           // stay within the f<C* contour
			uint64_t h2 = env.GetStateHash(n);
			auto jt = closed.find(h2);
			if (jt != closed.end() && flesseq(jt->second, g2)) continue;
			open.push(QN{f2, g2, n});
		}
	}
}

// |VC| = min_tau ( #{g_F < tau} + #{g_B < C*-tau} ), scanning tau over the
// distinct forward g-values (plus "cover all forward").
static uint64_t MinVertexCover(std::vector<double> fwd, std::vector<double> bwd, double Cstar)
{
	std::sort(fwd.begin(), fwd.end());
	std::sort(bwd.begin(), bwd.end());
	const double eps = 1e-7;

	// bwdCountLess(x) = # backward g-values strictly < x
	auto bwdCountLess = [&](double x) -> uint64_t {
		// first index with bwd[i] >= x-eps  (i.e. not strictly < x)
		size_t lo = 0, hi = bwd.size();
		while (lo < hi) { size_t m = (lo + hi) / 2; if (bwd[m] < x - eps) lo = m + 1; else hi = m; }
		return (uint64_t)lo;
	};

	uint64_t best = UINT64_MAX;
	// k = number of smallest forward nodes covered from the forward side.
	// The smallest uncovered forward g-value is fwd[k]; backward must then
	// cover all g_B < C* - fwd[k]. k = fwd.size() means cover all forward.
	size_t i = 0;
	while (i <= fwd.size())
	{
		uint64_t fwdCost = (uint64_t)i;
		uint64_t bwdCost;
		if (i == fwd.size()) bwdCost = 0;
		else                 bwdCost = bwdCountLess(Cstar - fwd[i]);
		uint64_t total = fwdCost + bwdCost;
		if (total < best) best = total;
		// advance past all forward nodes sharing fwd[i] (same threshold)
		if (i == fwd.size()) break;
		double v = fwd[i];
		while (i < fwd.size() && !(fwd[i] > v + eps)) i++;
	}
	return best;
}

int main(int argc, char *argv[])
{
	if (argc < 3) { printf("Usage: %s <map.3dmap> <scen.3dscen> [--limit N] [--no-diagonals]\n", argv[0]); return 1; }
	const char *mapFile = argv[1];
	const char *scenFile = argv[2];
	int limit = 50;
	int startIdx = 0;
	bool diagonals = true;
	double hweight = 1.0;
	for (int i = 3; i < argc; i++)
	{
		if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) limit = atoi(argv[++i]);
		else if (strcmp(argv[i], "--no-diagonals") == 0) diagonals = false;
		else if (strcmp(argv[i], "--hweight") == 0 && i + 1 < argc) hweight = atof(argv[++i]);
		else if (strcmp(argv[i], "--start") == 0 && i + 1 < argc) startIdx = atoi(argv[++i]);
	}

	VoxelMap env(mapFile, diagonals);
	env.SetHWeight(hweight);   // floor must use the same heuristic as the algorithms

	FILE *f = fopen(scenFile, "r");
	if (f == 0) { printf("Cannot open scenario '%s'\n", scenFile); return 1; }
	int version; char mapname[512];
	if (fscanf(f, "version %d\n", &version) != 1 || fscanf(f, "%511s\n", mapname) != 1)
	{ printf("Bad scenario header\n"); return 1; }

	TemplateAStar<voxState, voxAction, VoxelMap> astar;
	std::vector<voxState> path;

	printf("instance,cstar,fwd_cand,bwd_cand,mvc\n");
	char line[512];
	int idx = 0, done = 0;
	while (fgets(line, sizeof(line), f) && done < limit)
	{
		int sx, sy, sz, gx, gy, gz; double optCost;
		if (sscanf(line, "%d %d %d %d %d %d %lf", &sx, &sy, &sz, &gx, &gy, &gz, &optCost) != 7)
			continue;
		if (idx < startIdx) { idx++; continue; }   // skip to --start
		voxState s((uint16_t)sx, (uint16_t)sy, (uint16_t)sz), g((uint16_t)gx, (uint16_t)gy, (uint16_t)gz);

		// Exact C* from A* (the scenario cost is rounded to ~6 sig figs).
		astar.GetPath(&env, s, g, path);
		double Cstar = env.GetPathLength(path);

		std::vector<double> fwdG, bwdG;
		Contour(env, s, g, Cstar, fwdG);   // forward: heuristic toward goal
		Contour(env, g, s, Cstar, bwdG);   // backward: heuristic toward start
		uint64_t mvc = MinVertexCover(fwdG, bwdG, Cstar);

		printf("%d,%.6f,%zu,%zu,%llu\n", idx, Cstar, fwdG.size(), bwdG.size(), (unsigned long long)mvc);
		fflush(stdout);
		idx++; done++;
	}
	fclose(f);
	return 0;
}
