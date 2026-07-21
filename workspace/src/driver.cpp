//
//  driver.cpp
//  Headless, cluster-safe experiment driver: run bidirectional & unidirectional
//  search algorithms on Warthog voxel maps inside HOG2 and record metrics.
//
//  Each (instance, algorithm) run is executed in a forked child so that a single
//  pathological instance cannot hang the job or OOM the node:
//    * --timeout S  : wall-clock cap per run; the child is SIGKILLed on overrun.
//    * --mem-mb M   : RLIMIT_AS cap per child; overruns fail cleanly (status=oom/error).
//  The map is loaded once in the parent and shared copy-on-write with every child,
//  so forking is cheap even for billion-voxel maps.
//
//  Usage:
//    driver <map.3dmap> <scen.3dscen>
//           [--limit N] [--start K] [--no-diagonals]
//           [--algs astar,rastar,mm,bae,nbs,gbfs]
//           [--timeout SECONDS] [--mem-mb MB] [--tag STR]
//
//  CSV (stdout): tag,instance,alg,expanded,generated,cost,optimal,time_ms,status
//    status ∈ ok | subopt | timeout | oom | error | nopath
//
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <set>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include "VoxelMap.h"
#include "BiAStar.h"
#include "Timer.h"
#include "TemplateAStar.h"
#include "MM.h"
#include "BAE.h"
#include "NBS.h"
#include "BidirectionalGreedyBestFirst.h"

struct Instance { voxState s, g; double optimal; };

static bool costOK(double got, double opt) { return fabs(got - opt) < 1e-2; }

static double nowSeconds()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Run one algorithm on one instance and print its CSV row. Called inside the child.
// scenarioValid: whether the scenario's optimal-cost column applies to this run.
// It only does in diagonal (26-connected) mode; the benchmark's optimal costs are
// computed with diagonals, so in 6-connected mode we cannot validate against them
// (optimality is instead cross-checked via algorithm agreement in aggregate.py).
static void runOne(VoxelMap &env, const char *tag, int idx, const std::string &alg,
				   const Instance &in, bool scenarioValid)
{
	voxState s = in.s, g = in.g;
	double opt = in.optimal;
	std::vector<voxState> path, bpath;
	Timer t;
	uint64_t expanded = 0, generated = 0;
	double cost = 0;
	bool haveCost = true, optimalExpected = true;

	if (alg == "astar") {
		TemplateAStar<voxState, voxAction, VoxelMap> a;
		t.StartTimer(); a.GetPath(&env, s, g, path); t.EndTimer();
		expanded = a.GetNodesExpanded(); generated = a.GetNodesTouched(); cost = env.GetPathLength(path);
	} else if (alg == "rastar") {
		TemplateAStar<voxState, voxAction, VoxelMap> a;
		t.StartTimer(); a.GetPath(&env, g, s, path); t.EndTimer();
		expanded = a.GetNodesExpanded(); generated = a.GetNodesTouched(); cost = env.GetPathLength(path);
	} else if (alg == "mm") {
		MM<voxState, voxAction, VoxelMap> a;
		t.StartTimer(); a.GetPath(&env, s, g, &env, &env, path); t.EndTimer();
		expanded = a.GetNodesExpanded(); generated = a.GetNodesTouched(); cost = env.GetPathLength(path);
	} else if (alg == "bae") {
		// gcd=1e-6 disables the invalid gcd round-up for incommensurable {1,sqrt2,sqrt3} costs.
		BAE<voxState, voxAction, VoxelMap> a(true, 1.0, 1e-6);
		t.StartTimer(); a.GetPath(&env, s, g, &env, &env, path); t.EndTimer();
		expanded = a.GetNodesExpanded(); generated = a.GetNodesTouched(); cost = env.GetPathLength(path);
	} else if (alg == "nbs") {
		NBS<voxState, voxAction, VoxelMap> a;
		t.StartTimer(); a.GetPath(&env, s, g, &env, &env, path); t.EndTimer();
		expanded = a.GetNodesExpanded(); generated = a.GetNodesTouched(); cost = env.GetPathLength(path);
	} else if (alg == "bia") {
		BiAStar<voxState, voxAction, VoxelMap> a;
		t.StartTimer(); a.GetPath(&env, s, g, &env, &env, path); t.EndTimer();
		expanded = a.GetNodesExpanded(); generated = a.GetNodesTouched(); cost = env.GetPathLength(path);
	} else if (alg == "gbfs") {
		BidirectionalGreedyBestFirst<voxState, voxAction, VoxelMap> a;
		t.StartTimer(); a.GetPath(&env, s, g, path, bpath); t.EndTimer();
		expanded = (uint64_t)a.GetNodesExpanded(); generated = 0;
		cost = env.GetPathLength(path) + env.GetPathLength(bpath);
		optimalExpected = false; // GBFS is not cost-optimal
	} else {
		return;
	}

	const char *status;
	if (path.size() == 0 && (alg != "gbfs" || bpath.size() == 0)) status = "nopath";
	else if (!optimalExpected)                                    status = "subopt";  // GBFS
	else if (!scenarioValid)                                      status = "ok";      // nodiag: completed; not scenario-checked
	else if (costOK(cost, opt))                                   status = "ok";
	else                                                          status = "subopt";

	printf("%s,%d,%s,%llu,%llu,%.6f,%.6f,%.3f,%s\n",
		   tag, idx, alg.c_str(),
		   (unsigned long long)expanded, (unsigned long long)generated,
		   cost, opt, t.GetElapsedTime() * 1000.0, status);
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <map.3dmap> <scen.3dscen> [--limit N] [--start K] "
				"[--no-diagonals] [--algs a,b,..] [--timeout S] [--mem-mb M] [--tag STR]\n", argv[0]);
		return 1;
	}
	const char *mapFile = argv[1];
	const char *scenFile = argv[2];
	int limit = 1 << 30, startIdx = 0;
	bool diagonals = true;
	double timeoutSec = 0;      // 0 = no timeout
	long memMB = 0;             // 0 = no cap
	std::string tag = "";
	std::vector<std::string> algs = {"astar", "rastar", "mm", "bia", "bae", "nbs", "gbfs"};

	for (int i = 3; i < argc; i++) {
		if      (!strcmp(argv[i], "--limit")   && i+1 < argc) limit = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--start")   && i+1 < argc) startIdx = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--no-diagonals"))          diagonals = false;
		else if (!strcmp(argv[i], "--timeout") && i+1 < argc) timeoutSec = atof(argv[++i]);
		else if (!strcmp(argv[i], "--mem-mb")  && i+1 < argc) memMB = atol(argv[++i]);
		else if (!strcmp(argv[i], "--tag")     && i+1 < argc) tag = argv[++i];
		else if (!strcmp(argv[i], "--algs")    && i+1 < argc) {
			algs.clear();
			std::string a(argv[++i]); size_t p = 0, q;
			do { q = a.find(',', p); algs.push_back(a.substr(p, q - p)); p = q + 1; } while (q != std::string::npos);
		}
	}

	VoxelMap env(mapFile, diagonals);

	FILE *f = fopen(scenFile, "r");
	if (f == 0) { fprintf(stderr, "Cannot open scenario '%s'\n", scenFile); return 1; }
	int version; char mapname[512];
	if (fscanf(f, "version %d\n", &version) != 1 || fscanf(f, "%511s\n", mapname) != 1)
	{ fprintf(stderr, "Bad scenario header\n"); return 1; }

	std::vector<Instance> instances;
	{
		int sx, sy, sz, gx, gy, gz; double cost; char line[512];
		while (fgets(line, sizeof(line), f)) {
			if (sscanf(line, "%d %d %d %d %d %d %lf", &sx, &sy, &sz, &gx, &gy, &gz, &cost) == 7) {
				Instance in;
				in.s = voxState((uint16_t)sx, (uint16_t)sy, (uint16_t)sz);
				in.g = voxState((uint16_t)gx, (uint16_t)gy, (uint16_t)gz);
				in.optimal = cost;
				instances.push_back(in);
			}
		}
	}
	fclose(f);
	fprintf(stderr, "Loaded %zu instances (map v%d %s, tag='%s', timeout=%.0fs, mem=%ldMB)\n",
			instances.size(), version, mapname, tag.c_str(), timeoutSec, memMB);

	printf("tag,instance,alg,expanded,generated,cost,optimal,time_ms,status\n");
	fflush(stdout);

	int last = std::min((int)instances.size(), startIdx + limit);
	for (int i = startIdx; i < last; i++) {
		for (const auto &alg : algs) {
			fflush(stdout);               // flush parent buffer before fork (no dup output)
			pid_t pid = fork();
			if (pid == 0) {               // ---- child ----
				if (memMB > 0) {
					struct rlimit rl; rl.rlim_cur = rl.rlim_max = (rlim_t)memMB * 1024 * 1024;
					setrlimit(RLIMIT_AS, &rl);
				}
				runOne(env, tag.c_str(), i, alg, instances[i], diagonals);
				fflush(stdout);
				_exit(0);
			}
			// ---- parent: enforce wall-clock timeout ----
			double t0 = nowSeconds();
			int status; pid_t r;
			bool killed = false;
			while ((r = waitpid(pid, &status, WNOHANG)) == 0) {
				if (timeoutSec > 0 && nowSeconds() - t0 > timeoutSec) {
					kill(pid, SIGKILL); waitpid(pid, &status, 0); killed = true; break;
				}
				struct timespec ns{0, 20 * 1000 * 1000}; nanosleep(&ns, nullptr); // 20ms poll
			}
			// Parent reports only failures; the child prints its own row on success.
			if (killed) {
				printf("%s,%d,%s,0,0,0,%.6f,%.3f,timeout\n",
					   tag.c_str(), i, alg.c_str(), instances[i].optimal, timeoutSec * 1000.0);
				fflush(stdout);
			} else if (WIFSIGNALED(status)) {
				const char *st = (WTERMSIG(status) == SIGKILL) ? "oom" : "error";
				printf("%s,%d,%s,0,0,0,%.6f,0,%s\n", tag.c_str(), i, alg.c_str(), instances[i].optimal, st);
				fflush(stdout);
			} else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
				printf("%s,%d,%s,0,0,0,%.6f,0,error\n", tag.c_str(), i, alg.c_str(), instances[i].optimal);
				fflush(stdout);
			}
		}
	}
	return 0;
}
