//
//  VoxelMap.h
//  Headless voxel search environment for the Warthog 3D benchmarks.
//
//  Loads the Warthog `.3dmap` text format (both `voxel` and `rev_voxel`
//  polarities) and implements the HOG2 SearchEnvironment interface so that
//  every HOG2 search algorithm (A*, MM, BAE*, NBS, GBFS, ...) can run on it
//  with no GUI/OpenGL dependency.
//
//  The move model, edge costs, and heuristic are copied verbatim from HOG2's
//  environments/VoxelGrid.cpp so behaviour is identical to the reference
//  implementation, with one addition: an `allowDiagonals` flag that restricts
//  movement to the 6 face-neighbours (the brief's "without diagonal movement"
//  mode). With diagonals enabled it is the full 26-connected model with the
//  no-corner-cutting rule; costs are 1 / sqrt(2) / sqrt(3).
//

#ifndef VoxelMap_h
#define VoxelMap_h

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <cmath>
#include <functional>
#include "SearchEnvironment.h"

struct voxState {
	voxState() {}
	voxState(uint16_t a, uint16_t b, uint16_t c) : x(a), y(b), z(c) {}
	uint16_t x, y, z;
};

static inline bool operator==(const voxState &a, const voxState &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

namespace std {
	template <>
	struct hash<voxState> {
		std::size_t operator()(const voxState &k) const
		{
			return ((size_t)k.x << 32) | ((size_t)k.y << 16) | (size_t)k.z;
		}
	};
}

// Action encodes the (dx,dy,dz) offset, each in {-1,0,1}, packed as in VoxelGrid.
typedef uint8_t voxAction;

class VoxelMap : public SearchEnvironment<voxState, voxAction> {
public:
	// Load from a Warthog .3dmap text file (uncompressed). Handles `voxel`
	// (listed = obstacles) and `rev_voxel` (listed = free space).
	explicit VoxelMap(const char *filename, bool allowDiagonals = true)
	: diagonals(allowDiagonals)
	{
		FILE *f = fopen(filename, "r");
		if (f == 0) { printf("VoxelMap: error opening '%s'\n", filename); exit(1); }

		char type[64];
		int cnt = fscanf(f, "%63s %d %d %d\n", type, &xWidth, &yWidth, &zWidth);
		if (cnt != 4) { printf("VoxelMap: bad header in '%s'\n", filename); exit(1); }

		std::string t(type);
		bool listedAreObstacles;
		if (t == "voxel")            listedAreObstacles = true;
		else if (t == "rev_voxel")   listedAreObstacles = false;
		else { printf("VoxelMap: unknown map type '%s'\n", type); exit(1); }

		// For `voxel`: default free (false), set listed = blocked.
		// For `rev_voxel`: default blocked (true), set listed = free.
		blocked.assign((size_t)xWidth * yWidth * zWidth, !listedAreObstacles);

		int a, b, c;
		size_t nListed = 0;
		while ((cnt = fscanf(f, "%d %d %d\n", &a, &b, &c)) == 3)
		{
			if (a >= 0 && a < xWidth && b >= 0 && b < yWidth && c >= 0 && c < zWidth)
				blocked[Index(a, b, c)] = listedAreObstacles;
			nListed++;
		}
		fclose(f);
		fprintf(stderr, "VoxelMap: loaded '%s' [%s] %dx%dx%d, %zu listed voxels, diagonals=%d\n",
				filename, type, xWidth, yWidth, zWidth, nListed, (int)diagonals);
	}

	void SetDiagonals(bool d) { diagonals = d; }
	void GetLimits(int &x, int &y, int &z) const { x = xWidth; y = yWidth; z = zWidth; }
	uint64_t NumVoxels() const { return (uint64_t)xWidth * yWidth * zWidth; }

	bool IsBlocked(int x, int y, int z) const
	{
		if (x < 0 || y < 0 || z < 0 || x >= xWidth || y >= yWidth || z >= zWidth)
			return true; // out of bounds treated as wall
		return blocked[Index(x, y, z)];
	}
	bool IsBlocked(const voxState &s) const { return IsBlocked(s.x, s.y, s.z); }

	// A move from s1 to s2 (face/edge/corner neighbours) is legal only if both
	// endpoints are free and no clipped voxel is blocked (no-corner-cutting).
	bool CanMove(const voxState &s1, const voxState &s2) const
	{
		return (IsBlocked(s1) ||
				IsBlocked(s2) ||
				IsBlocked(s2.x, s1.y, s1.z) ||
				IsBlocked(s1.x, s2.y, s1.z) ||
				IsBlocked(s1.x, s1.y, s2.z) ||
				IsBlocked(s2.x, s2.y, s1.z) ||
				IsBlocked(s2.x, s1.y, s2.z) ||
				IsBlocked(s1.x, s2.y, s2.z)) == false;
	}

	void GetSuccessors(const voxState &nodeID, std::vector<voxState> &neighbors) const override
	{
		neighbors.resize(0);
		for (int x = -1; x <= 1; x++)
			for (int y = -1; y <= 1; y++)
				for (int z = -1; z <= 1; z++)
				{
					if ((x | y | z) == 0) continue;                 // no-op
					if (!diagonals && (abs(x) + abs(y) + abs(z)) != 1) continue; // faces only
					voxState s((uint16_t)(nodeID.x + x), (uint16_t)(nodeID.y + y), (uint16_t)(nodeID.z + z));
					if (CanMove(nodeID, s))
						neighbors.push_back(s);
				}
	}

	void GetActions(const voxState &nodeID, std::vector<voxAction> &actions) const override
	{
		actions.resize(0);
		for (int x = -1; x <= 1; x++)
			for (int y = -1; y <= 1; y++)
				for (int z = -1; z <= 1; z++)
				{
					if ((x | y | z) == 0) continue;
					if (!diagonals && (abs(x) + abs(y) + abs(z)) != 1) continue;
					voxState s((uint16_t)(nodeID.x + x), (uint16_t)(nodeID.y + y), (uint16_t)(nodeID.z + z));
					if (CanMove(nodeID, s))
						actions.push_back(MakeAction(x, y, z));
				}
	}

	void ApplyAction(voxState &s, voxAction a) const override
	{
		int x = ((a >> 4) & 3) - 1;
		int y = ((a >> 2) & 3) - 1;
		int z = ((a >> 0) & 3) - 1;
		s.x += x; s.y += y; s.z += z;
	}

	bool InvertAction(voxAction &a) const override { return false; }

	// Scale the heuristic by w in (0,1] to make it deliberately weaker while staying
	// admissible+consistent (h' = w*h <= h <= true distance). Used for the
	// heuristic-strength crossover study; w=1 is the normal octile, w=0 is Dijkstra.
	void SetHWeight(double w) { hweight = w; }

	// 3D octile heuristic (generalisation of 2D octile): consistent.
	double HCost(const voxState &n1, const voxState &n2) const override
	{
		double xd = fabs((double)n1.x - n2.x);
		double yd = fabs((double)n1.y - n2.y);
		double zd = fabs((double)n1.z - n2.z);
		double h;
		if (!diagonals) {                     // Manhattan when no diagonals
			h = xd + yd + zd;
		} else {
			double three = std::min(xd, std::min(yd, zd));
			xd -= three; yd -= three; zd -= three;
			double two;
			if (zd == 0)      { two = std::min(xd, yd); xd -= two; yd -= two; }
			else if (xd == 0) { two = std::min(zd, yd); zd -= two; yd -= two; }
			else              { two = std::min(xd, zd); xd -= two; zd -= two; }
			h = three * ROOT_THREE + two * ROOT_TWO + xd + yd + zd;
		}
		return hweight * h;
	}

	double GCost(const voxState &n1, const voxState &n2) const override
	{
		int diff = (n1.x != n2.x) + (n1.y != n2.y) + (n1.z != n2.z);
		static const double v[4] = {0, 1, ROOT_TWO, ROOT_THREE};
		return v[diff];
	}

	double GCost(const voxState &node, const voxAction &act) const override
	{
		int x = ((act >> 4) & 3) - 1;
		int y = ((act >> 2) & 3) - 1;
		int z = ((act >> 0) & 3) - 1;
		int diff = (x != 0) + (y != 0) + (z != 0);
		static const double v[4] = {0, 1, ROOT_TWO, ROOT_THREE};
		return v[diff];
	}

	bool GoalTest(const voxState &node, const voxState &goal) const override { return node == goal; }

	uint64_t GetStateHash(const voxState &node) const override
	{
		return ((uint64_t)node.x << 32) | ((uint64_t)node.y << 16) | (uint64_t)node.z;
	}
	void GetStateFromHash(uint64_t h, voxState &s) const
	{
		s.z = h & 0xFFFF; s.y = (h >> 16) & 0xFFFF; s.x = (h >> 32) & 0xFFFF;
	}
	uint64_t GetActionHash(voxAction act) const override { return (uint64_t)act; }

private:
	size_t Index(int x, int y, int z) const { return (size_t)xWidth * ((size_t)y * zWidth + z) + x; }
	voxAction MakeAction(int x, int y, int z) const
	{
		voxAction v = 0;
		v |= (((x + 1) & 3) << 4);
		v |= (((y + 1) & 3) << 2);
		v |= (((z + 1) & 3) << 0);
		return v;
	}

	std::vector<bool> blocked;
	int xWidth = 0, yWidth = 0, zWidth = 0;
	bool diagonals = true;
	double hweight = 1.0;   // heuristic scale (1 = octile, <1 = weaker but still admissible)
};

#endif /* VoxelMap_h */
