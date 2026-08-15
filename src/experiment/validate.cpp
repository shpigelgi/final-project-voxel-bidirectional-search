//
//  validate.cpp
//  Independent legality checker for the voxel move model. Verifies that:
//   (A) the successor generator never produces an illegal step, and
//   (B) every solution path returned by every algorithm consists only of legal
//       steps — neighbor-only, endpoints free, and NO corner-cutting.
//
//  The legality test below is re-implemented here from scratch (it does NOT call
//  VoxelMap::CanMove), so it is a genuine cross-check of the search, not circular.
//
//  Usage: validate <map.3dmap> <scen.3dscen> [--limit N] [--no-diagonals]
//
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include "../core/VoxelMap.h"
#include "TemplateAStar.h"
#include "MM.h"
#include "BAE.h"
#include "NBS.h"
#include "../core/BiAStar.h"

// Independent legality check for a single step a->b under the voxel move model.
static bool legalStep(const VoxelMap &env, const voxState &a, const voxState &b, bool diag, const char **why)
{
	int dx = std::abs((int)a.x-(int)b.x), dy = std::abs((int)a.y-(int)b.y), dz = std::abs((int)a.z-(int)b.z);
	int cheb = std::max(dx, std::max(dy, dz)), manh = dx+dy+dz;
	if (cheb != 1)            { *why = "not-a-neighbor"; return false; }        // must move to an adjacent voxel
	if (!diag && manh != 1)   { *why = "diagonal-in-6conn"; return false; }     // face moves only when diagonals off
	if (env.IsBlocked(a.x,a.y,a.z)) { *why = "from-blocked"; return false; }
	if (env.IsBlocked(b.x,b.y,b.z)) { *why = "into-blocked"; return false; }
	// no-corner-cutting: every voxel the diagonal clips must be free
	if (env.IsBlocked(b.x,a.y,a.z)) { *why="clip"; return false; }
	if (env.IsBlocked(a.x,b.y,a.z)) { *why="clip"; return false; }
	if (env.IsBlocked(a.x,a.y,b.z)) { *why="clip"; return false; }
	if (env.IsBlocked(b.x,b.y,a.z)) { *why="clip"; return false; }
	if (env.IsBlocked(b.x,a.y,b.z)) { *why="clip"; return false; }
	if (env.IsBlocked(a.x,b.y,b.z)) { *why="clip"; return false; }
	return true;
}

int main(int argc, char *argv[])
{
	if (argc < 3) { fprintf(stderr, "Usage: %s <map> <scen> [--limit N] [--no-diagonals]\n", argv[0]); return 1; }
	const char *mapFile = argv[1], *scenFile = argv[2];
	int limit = 50; bool diag = true;
	for (int i = 3; i < argc; i++) {
		if (!strcmp(argv[i], "--limit") && i+1 < argc) limit = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--no-diagonals")) diag = false;
	}
	VoxelMap env(mapFile, diag);
	int W,H,D; env.GetLimits(W,H,D);

	// ---- (A) audit the successor generator on many free cells ----
	long succChecked = 0, succBad = 0;
	std::vector<voxState> succ;
	srandom(12345);
	for (long tries = 0, got = 0; got < 20000 && tries < 4000000; tries++) {
		voxState c((uint16_t)(random()%W),(uint16_t)(random()%H),(uint16_t)(random()%D));
		if (env.IsBlocked(c.x,c.y,c.z)) continue;
		got++;
		env.GetSuccessors(c, succ);
		for (const auto &s : succ) { const char *why="";
			succChecked++;
			if (!legalStep(env, c, s, diag, &why)) { succBad++;
				if (succBad<=5) fprintf(stderr,"  SUCC illegal (%d,%d,%d)->(%d,%d,%d): %s\n",c.x,c.y,c.z,s.x,s.y,s.z,why);
			}
		}
	}

	// ---- (B) audit every returned path, per algorithm ----
	FILE *f = fopen(scenFile, "r"); if (!f) { fprintf(stderr,"no scen\n"); return 1; }
	int ver; char mn[512]; fscanf(f,"version %d\n",&ver); fscanf(f,"%511s\n",mn);
	struct Inst{ voxState s,g; }; std::vector<Inst> inst;
	{ int sx,sy,sz,gx,gy,gz; double c; char ln[512];
	  while (fgets(ln,sizeof ln,f) && (int)inst.size()<limit)
	    if (sscanf(ln,"%d %d %d %d %d %d %lf",&sx,&sy,&sz,&gx,&gy,&gz,&c)==7)
	      inst.push_back({{(uint16_t)sx,(uint16_t)sy,(uint16_t)sz},{(uint16_t)gx,(uint16_t)gy,(uint16_t)gz}}); }
	fclose(f);

	const char *algs[] = {"astar","rastar","mm","bia","bae","nbs"};
	long pathEdges=0, pathBad=0, pathsChecked=0, pathDup=0;
	std::vector<voxState> path;
	for (auto &in : inst) {
		for (const char *alg : algs) {
			path.clear();
			if      (!strcmp(alg,"astar")) { TemplateAStar<voxState,voxAction,VoxelMap> a; a.GetPath(&env,in.s,in.g,path); }
			else if (!strcmp(alg,"rastar")){ TemplateAStar<voxState,voxAction,VoxelMap> a; a.GetPath(&env,in.g,in.s,path); }
			else if (!strcmp(alg,"mm"))    { MM<voxState,voxAction,VoxelMap> a; a.GetPath(&env,in.s,in.g,&env,&env,path); }
			else if (!strcmp(alg,"bia"))   { BiAStar<voxState,voxAction,VoxelMap> a; a.GetPath(&env,in.s,in.g,&env,&env,path); }
			else if (!strcmp(alg,"bae"))   { BAE<voxState,voxAction,VoxelMap> a(true,1.0,1e-6); a.GetPath(&env,in.s,in.g,&env,&env,path); }
			else if (!strcmp(alg,"nbs"))   { NBS<voxState,voxAction,VoxelMap> a; a.GetPath(&env,in.s,in.g,&env,&env,path); }
			if (path.size()<2) continue;
			pathsChecked++;
			for (size_t i=0;i+1<path.size();i++){ const char *why="";
				if (path[i]==path[i+1]) { pathDup++; continue; }   // benign duplicate meeting node
				pathEdges++;
				if (!legalStep(env, path[i], path[i+1], diag, &why)) { pathBad++;
					if (pathBad<=8) fprintf(stderr,"  PATH illegal [%s] (%d,%d,%d)->(%d,%d,%d): %s\n",
						alg,path[i].x,path[i].y,path[i].z,path[i+1].x,path[i+1].y,path[i+1].z,why);
				}
			}
		}
	}

	printf("diag=%d | successors: %ld checked, %ld ILLEGAL | paths: %ld checked, %ld edges, %ld ILLEGAL, %ld dup-node | %s\n",
		   (int)diag, succChecked, succBad, pathsChecked, pathEdges, pathBad, pathDup, mapFile);
	return (succBad || pathBad) ? 2 : 0;
}
