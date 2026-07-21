//
//  trace.cpp
//  Emit a JSON trace of one search (one map, one instance, one algorithm) for
//  the interactive visualizer: every expanded (closed) node tagged with its
//  side (forward/backward) and its expansion-priority key, plus the final path.
//  Replaying the nodes sorted by key reproduces the expansion order.
//
//  Usage:
//    trace <map.3dmap> <scen.3dscen> --instance K --alg {astar|mm|bia|bae}
//          [--no-diagonals]
//  JSON is written to stdout.
//
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include "VoxelMap.h"
#include "BiAStar.h"
#include "TemplateAStar.h"
#include "MM.h"
#include "BAE.h"

struct Node { int x, y, z; double g, key; int side; }; // side 0=forward,1=backward

static void emit(const VoxelMap &env, int W, int H, int D,
				 const voxState &s, const voxState &g, const std::string &alg, bool diag,
				 double cost, const std::vector<Node> &nodes, const std::vector<voxState> &path)
{
	printf("{\n");
	printf("  \"alg\": \"%s\", \"diag\": %s,\n", alg.c_str(), diag ? "true" : "false");
	printf("  \"dims\": [%d,%d,%d],\n", W, H, D);
	printf("  \"start\": [%d,%d,%d], \"goal\": [%d,%d,%d],\n", s.x, s.y, s.z, g.x, g.y, g.z);
	printf("  \"cost\": %.6f, \"expanded\": %zu,\n", cost, nodes.size());
	// nodes: [x,y,z,side,key]
	printf("  \"nodes\": [");
	for (size_t i = 0; i < nodes.size(); i++)
		printf("%s[%d,%d,%d,%d,%.4f]", i ? "," : "", nodes[i].x, nodes[i].y, nodes[i].z,
			   nodes[i].side, nodes[i].key);
	printf("],\n");
	printf("  \"path\": [");
	for (size_t i = 0; i < path.size(); i++)
		printf("%s[%d,%d,%d]", i ? "," : "", path[i].x, path[i].y, path[i].z);
	printf("]\n}\n");
}

int main(int argc, char *argv[])
{
	if (argc < 3) { fprintf(stderr, "Usage: %s <map> <scen> --instance K --alg X [--no-diagonals]\n", argv[0]); return 1; }
	const char *mapFile = argv[1], *scenFile = argv[2];
	int inst = 0; std::string alg = "mm"; bool diag = true;
	for (int i = 3; i < argc; i++) {
		if      (!strcmp(argv[i], "--instance") && i+1 < argc) inst = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--alg")      && i+1 < argc) alg = argv[++i];
		else if (!strcmp(argv[i], "--no-diagonals"))           diag = false;
	}

	VoxelMap env(mapFile, diag);
	int W, H, D; env.GetLimits(W, H, D);

	FILE *f = fopen(scenFile, "r");
	if (!f) { fprintf(stderr, "cannot open scen\n"); return 1; }
	int ver; char mn[512]; fscanf(f, "version %d\n", &ver); fscanf(f, "%511s\n", mn);
	voxState s, g; double opt = 0; char line[512]; int idx = 0; bool found = false;
	while (fgets(line, sizeof line, f)) {
		int sx,sy,sz,gx,gy,gz; double c;
		if (sscanf(line, "%d %d %d %d %d %d %lf", &sx,&sy,&sz,&gx,&gy,&gz,&c) == 7) {
			if (idx == inst) { s={(uint16_t)sx,(uint16_t)sy,(uint16_t)sz}; g={(uint16_t)gx,(uint16_t)gy,(uint16_t)gz}; opt=c; found=true; break; }
			idx++;
		}
	}
	fclose(f);
	if (!found) { fprintf(stderr, "instance %d not found\n", inst); return 1; }

	std::vector<Node> nodes;
	std::vector<voxState> path;
	double cost = 0;

	auto pushUni = [&](auto &alg_obj) {
		for (int i = 0; i < alg_obj.GetNumItems(); i++) {
			const auto &it = alg_obj.GetItem(i);
			if (it.where == kClosedList)
				nodes.push_back({it.data.x, it.data.y, it.data.z, it.g, it.f, 0});
		}
	};
	auto pushBiMM = [&](auto &alg_obj, bool baeKey) {
		for (int i = 0; i < alg_obj.GetNumForwardItems(); i++) {
			const auto &it = alg_obj.GetForwardItem(i);
			if (it.where == kClosedList) {
				double key = baeKey ? it.h : std::max(it.g + it.h, 2 * it.g); // BAE: .h is b-value; MM: max(f,2g)
				nodes.push_back({it.data.x, it.data.y, it.data.z, it.g, key, 0});
			}
		}
		for (int i = 0; i < alg_obj.GetNumBackwardItems(); i++) {
			const auto &it = alg_obj.GetBackwardItem(i);
			if (it.where == kClosedList) {
				double key = baeKey ? it.h : std::max(it.g + it.h, 2 * it.g);
				nodes.push_back({it.data.x, it.data.y, it.data.z, it.g, key, 1});
			}
		}
	};

	if (alg == "astar") {
		TemplateAStar<voxState, voxAction, VoxelMap> a;
		a.GetPath(&env, s, g, path); cost = env.GetPathLength(path); pushUni(a);
	} else if (alg == "mm") {
		MM<voxState, voxAction, VoxelMap> a;
		a.GetPath(&env, s, g, &env, &env, path); cost = env.GetPathLength(path); pushBiMM(a, false);
	} else if (alg == "bae") {
		BAE<voxState, voxAction, VoxelMap> a(true, 1.0, 1e-6);
		a.GetPath(&env, s, g, &env, &env, path); cost = env.GetPathLength(path); pushBiMM(a, true);
	} else if (alg == "bia") {
		BiAStar<voxState, voxAction, VoxelMap> a;
		a.GetPath(&env, s, g, &env, &env, path); cost = env.GetPathLength(path);
		// BiA*: .h stores the f priority for both sides.
		for (int i = 0; i < a.GetNumForwardItems(); i++) { const auto &it = a.GetForwardItem(i); if (it.where==kClosedList) nodes.push_back({it.data.x,it.data.y,it.data.z,it.g,it.h,0}); }
		for (int i = 0; i < a.GetNumBackwardItems(); i++){ const auto &it = a.GetBackwardItem(i); if (it.where==kClosedList) nodes.push_back({it.data.x,it.data.y,it.data.z,it.g,it.h,1}); }
	} else { fprintf(stderr, "unknown alg '%s'\n", alg.c_str()); return 1; }

	// order by expansion key so the animation plays back in expansion order
	std::stable_sort(nodes.begin(), nodes.end(), [](const Node &a, const Node &b){ return a.key < b.key; });
	emit(env, W, H, D, s, g, alg, diag, cost, nodes, path);
	fprintf(stderr, "traced %s inst %d: %zu expansions, cost %.4f (opt %.4f)\n",
			alg.c_str(), inst, nodes.size(), cost, opt);
	return 0;
}
