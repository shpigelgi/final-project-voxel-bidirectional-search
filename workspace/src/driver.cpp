//
//  driver.cpp
//  Headless experiment driver: run bidirectional & unidirectional search
//  algorithms on Warthog voxel maps inside HOG2 and record metrics.
//
//  Usage:
//    driver <map.3dmap> <scen.3dscen> [--limit N] [--no-diagonals]
//           [--algs astar,rastar,mm,bae,nbs,gbfs]
//
//  Emits one CSV row per (instance, algorithm):
//    instance,alg,expanded,generated,cost,optimal,time_ms,optimal_ok
//
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include "VoxelMap.h"
#include "Timer.h"
#include "TemplateAStar.h"
#include "MM.h"
#include "BAE.h"
#include "NBS.h"
#include "BidirectionalGreedyBestFirst.h"

struct Instance { voxState s, g; double optimal; };

static bool costOK(double got, double opt)
{
	return fabs(got - opt) < 1e-3;
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Usage: %s <map.3dmap> <scen.3dscen> [--limit N] [--no-diagonals] [--algs a,b,..]\n", argv[0]);
		return 1;
	}
	const char *mapFile = argv[1];
	const char *scenFile = argv[2];
	int limit = 50;
	bool diagonals = true;
	std::set<std::string> algs = {"astar", "rastar", "mm", "bae", "nbs", "gbfs"};

	for (int i = 3; i < argc; i++)
	{
		if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) limit = atoi(argv[++i]);
		else if (strcmp(argv[i], "--no-diagonals") == 0) diagonals = false;
		else if (strcmp(argv[i], "--algs") == 0 && i + 1 < argc)
		{
			algs.clear();
			std::string a(argv[++i]); size_t p = 0, q;
			do { q = a.find(',', p); algs.insert(a.substr(p, q - p)); p = q + 1; } while (q != std::string::npos);
		}
	}

	VoxelMap env(mapFile, diagonals);

	// --- parse scenario ---
	FILE *f = fopen(scenFile, "r");
	if (f == 0) { printf("Cannot open scenario '%s'\n", scenFile); return 1; }
	int version = 0;
	char mapname[512];
	if (fscanf(f, "version %d\n", &version) != 1) { printf("Bad scenario header\n"); return 1; }
	if (fscanf(f, "%511s\n", mapname) != 1) { printf("Bad scenario map line\n"); return 1; }
	fprintf(stderr, "Scenario version %d, map %s\n", version, mapname);

	std::vector<Instance> instances;
	{
		// Read up to `limit` instances. Each line: sx sy sz tx ty tz cost [herr] [dbias]
		// We read the 6 coords + optimal cost and ignore any trailing floats.
		int sx, sy, sz, gx, gy, gz;
		double cost;
		char line[512];
		while (fgets(line, sizeof(line), f) && (int)instances.size() < limit)
		{
			if (sscanf(line, "%d %d %d %d %d %d %lf", &sx, &sy, &sz, &gx, &gy, &gz, &cost) == 7)
			{
				Instance in;
				in.s = voxState((uint16_t)sx, (uint16_t)sy, (uint16_t)sz);
				in.g = voxState((uint16_t)gx, (uint16_t)gy, (uint16_t)gz);
				in.optimal = cost;
				instances.push_back(in);
			}
		}
	}
	fclose(f);
	fprintf(stderr, "Loaded %zu instances (limit %d)\n", instances.size(), limit);

	// --- algorithm objects ---
	TemplateAStar<voxState, voxAction, VoxelMap> astar;
	MM<voxState, voxAction, VoxelMap> mm;
	BAE<voxState, voxAction, VoxelMap> bae;
	NBS<voxState, voxAction, VoxelMap> nbs;
	BidirectionalGreedyBestFirst<voxState, voxAction, VoxelMap> gbfs;
	std::vector<voxState> path, bpath;
	Timer t;

	printf("instance,alg,expanded,generated,cost,optimal,time_ms,optimal_ok\n");

	for (size_t i = 0; i < instances.size(); i++)
	{
		voxState s = instances[i].s, g = instances[i].g;
		double opt = instances[i].optimal;

		#define ROW(alg, exp, gen, cost, ok) \
			printf("%zu,%s,%llu,%llu,%.6f,%.6f,%.3f,%d\n", i, alg, \
				   (unsigned long long)(exp), (unsigned long long)(gen), (double)(cost), opt, \
				   t.GetElapsedTime() * 1000.0, (ok) ? 1 : 0)

		if (algs.count("astar")) {
			t.StartTimer(); astar.GetPath(&env, s, g, path); t.EndTimer();
			double c = env.GetPathLength(path);
			ROW("astar", astar.GetNodesExpanded(), astar.GetNodesTouched(), c, costOK(c, opt));
		}
		if (algs.count("rastar")) {
			t.StartTimer(); astar.GetPath(&env, g, s, path); t.EndTimer();
			double c = env.GetPathLength(path);
			ROW("rastar", astar.GetNodesExpanded(), astar.GetNodesTouched(), c, costOK(c, opt));
		}
		if (algs.count("mm")) {
			t.StartTimer(); mm.GetPath(&env, s, g, &env, &env, path); t.EndTimer();
			double c = env.GetPathLength(path);
			ROW("mm", mm.GetNodesExpanded(), mm.GetNodesTouched(), c, costOK(c, opt));
		}
		if (algs.count("bae")) {
			t.StartTimer(); bae.GetPath(&env, s, g, &env, &env, path); t.EndTimer();
			double c = env.GetPathLength(path);
			ROW("bae", bae.GetNodesExpanded(), bae.GetNodesTouched(), c, costOK(c, opt));
		}
		if (algs.count("nbs")) {
			t.StartTimer(); nbs.GetPath(&env, s, g, &env, &env, path); t.EndTimer();
			double c = env.GetPathLength(path);
			ROW("nbs", nbs.GetNodesExpanded(), nbs.GetNodesTouched(), c, costOK(c, opt));
		}
		if (algs.count("gbfs")) {
			t.StartTimer(); gbfs.GetPath(&env, s, g, path, bpath); t.EndTimer();
			double c = env.GetPathLength(path) + env.GetPathLength(bpath);
			ROW("gbfs", (uint64_t)gbfs.GetNodesExpanded(), 0ULL, c, false /* not optimal */);
		}
	}
	return 0;
}
