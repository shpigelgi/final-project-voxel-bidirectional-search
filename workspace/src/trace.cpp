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
static size_t gMaxNodes = 0;      // 0 = emit all expansions; else stride-downsample to this many
static size_t gMaxObs   = 45000;  // cap on emitted obstacle voxels

static void emit(const VoxelMap &env, int W, int H, int D,
				 const voxState &s, const voxState &g, const std::string &alg, bool diag,
				 double cost, const std::vector<Node> &nodes, const std::vector<voxState> &path,
				 const std::vector<voxState> &obstacles)
{
	printf("{\n");
	printf("  \"alg\": \"%s\", \"diag\": %s,\n", alg.c_str(), diag ? "true" : "false");
	printf("  \"dims\": [%d,%d,%d],\n", W, H, D);
	printf("  \"start\": [%d,%d,%d], \"goal\": [%d,%d,%d],\n", s.x, s.y, s.z, g.x, g.y, g.z);
	printf("  \"cost\": %.6f, \"expanded\": %zu,\n", cost, nodes.size());
	// nodes: [x,y,z,side,key]. Optionally downsampled for display (stride over the
	// key-sorted order so the animation stays smooth and the file small).
	size_t maxN = gMaxNodes ? gMaxNodes : nodes.size();
	size_t stride = (nodes.size() > maxN) ? (nodes.size() / maxN) + 1 : 1;
	printf("  \"nodes\": [");
	bool first = true;
	for (size_t i = 0; i < nodes.size(); i += stride) {
		printf("%s[%d,%d,%d,%d,%.4f]", first ? "" : ",", nodes[i].x, nodes[i].y, nodes[i].z,
			   nodes[i].side, nodes[i].key);
		first = false;
	}
	printf("],\n");
	printf("  \"obstacles\": [");
	for (size_t i = 0; i < obstacles.size(); i++)
		printf("%s[%d,%d,%d]", i ? "," : "", obstacles[i].x, obstacles[i].y, obstacles[i].z);
	printf("],\n");
	printf("  \"path\": [");
	for (size_t i = 0; i < path.size(); i++)
		printf("%s[%d,%d,%d]", i ? "," : "", path[i].x, path[i].y, path[i].z);
	printf("]\n}\n");
}

// Collect obstacle voxels inside the bounding box of the search (nodes + path +
// endpoints), padded by a margin. Capped by striding so the trace stays small.
static std::vector<voxState> collectObstacles(const VoxelMap &env, int W, int H, int D,
		const std::vector<Node> &nodes, const std::vector<voxState> &path,
		const voxState &s, const voxState &g)
{
	int lo[3] = {W, H, D}, hi[3] = {0, 0, 0};
	auto grow = [&](int x, int y, int z){
		lo[0]=std::min(lo[0],x); hi[0]=std::max(hi[0],x);
		lo[1]=std::min(lo[1],y); hi[1]=std::max(hi[1],y);
		lo[2]=std::min(lo[2],z); hi[2]=std::max(hi[2],z);
	};
	for (const auto &n : nodes) grow(n.x, n.y, n.z);
	for (const auto &p : path)  grow(p.x, p.y, p.z);
	grow(s.x, s.y, s.z); grow(g.x, g.y, g.z);
	const int pad = 3;
	for (int i = 0; i < 3; i++) { lo[i] = std::max(0, lo[i]-pad); }
	hi[0]=std::min(W-1,hi[0]+pad); hi[1]=std::min(H-1,hi[1]+pad); hi[2]=std::min(D-1,hi[2]+pad);

	std::vector<voxState> obs;
	const size_t cap = gMaxObs;
	// two-pass: count, then stride to stay under cap
	size_t total = 0;
	for (int x=lo[0]; x<=hi[0]; x++) for (int y=lo[1]; y<=hi[1]; y++) for (int z=lo[2]; z<=hi[2]; z++)
		if (env.IsBlocked(x,y,z)) total++;
	int stride = (total > cap) ? (int)(total / cap) + 1 : 1;
	size_t seen = 0;
	for (int x=lo[0]; x<=hi[0]; x++) for (int y=lo[1]; y<=hi[1]; y++) for (int z=lo[2]; z<=hi[2]; z++)
		if (env.IsBlocked(x,y,z)) { if (seen++ % stride == 0) obs.push_back({(uint16_t)x,(uint16_t)y,(uint16_t)z}); }
	fprintf(stderr, "obstacles in bbox [%d-%d,%d-%d,%d-%d]: %zu total, %zu emitted (stride %d)\n",
			lo[0],hi[0],lo[1],hi[1],lo[2],hi[2], total, obs.size(), stride);
	return obs;
}

int main(int argc, char *argv[])
{
	if (argc < 3) { fprintf(stderr, "Usage: %s <map> <scen> --instance K --alg X [--no-diagonals]\n", argv[0]); return 1; }
	const char *mapFile = argv[1], *scenFile = argv[2];
	int inst = 0; std::string alg = "mm"; bool diag = true;
	for (int i = 3; i < argc; i++) {
		if      (!strcmp(argv[i], "--instance") && i+1 < argc) inst = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--alg")      && i+1 < argc) alg = argv[++i];
		else if (!strcmp(argv[i], "--max-nodes")&& i+1 < argc) gMaxNodes = (size_t)atol(argv[++i]);
		else if (!strcmp(argv[i], "--max-obs")  && i+1 < argc) gMaxObs   = (size_t)atol(argv[++i]);
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
	auto obstacles = collectObstacles(env, W, H, D, nodes, path, s, g);
	emit(env, W, H, D, s, g, alg, diag, cost, nodes, path, obstacles);
	fprintf(stderr, "traced %s inst %d: %zu expansions, cost %.4f (opt %.4f)\n",
			alg.c_str(), inst, nodes.size(), cost, opt);
	return 0;
}
