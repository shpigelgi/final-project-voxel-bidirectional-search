//
//  BiAStar.h
//  Pohl-style bidirectional A* (a.k.a. BS*) for front-to-end heuristics.
//
//  This is the "naive" bidirectional A* baseline that MM and BAE* are designed
//  to improve on: two A* searches (priority f = g + h toward the opposite
//  endpoint) meet, with no meet-in-the-middle cap. It is the natural reading of
//  the brief's "BiA*", and HOG2 has no such class, so we provide one here.
//
//  Structure and bookkeeping (open/closed lists, lazy nipping against the
//  opposite closed list, solution detection, path extraction) are adapted from
//  HOG2's BAE.h; only the node priority and the termination bound differ:
//    * priority  = g + h(n, target)                       (plain A* f-value)
//    * lower bnd = max(fMin_F, fMin_B)                     (classic BS* rule)
//  The rule is optimal: if a cheaper-than-incumbent solution existed, the first
//  still-open node on the optimal path (one side) would have f <= C* < U, so
//  fMin on that side would be < U; hence fMin_F >= U OR fMin_B >= U proves
//  the incumbent optimal.
//
#ifndef BiAStar_h
#define BiAStar_h

#include "AStarOpenClosed.h"
#include "FPUtil.h"
#include "Heuristic.h"
#include <vector>
#include <algorithm>
#include <cfloat>

template <class state>
struct BiACompare {
	bool operator()(const AStarOpenClosedData<state> &i1, const AStarOpenClosedData<state> &i2) const {
		// The node's .h field stores its priority (the A* f-value).
		double p1 = i1.h, p2 = i2.h;
		if (fequal(p1, p2))
			return fless(i1.g, i2.g);   // prefer higher g on ties
		return fgreater(p1, p2);        // min-priority at the top
	}
};

template <class state, class action, class environment,
		  class priorityQueue = AStarOpenClosed<state, BiACompare<state>>>
class BiAStar {
public:
	BiAStar(bool alternating_ = true) : alternating(alternating_) {
		forwardHeuristic = backwardHeuristic = 0; env = 0; ResetNodeCount();
	}
	virtual ~BiAStar() {}

	void GetPath(environment *e, const state &from, const state &to,
				 Heuristic<state> *forward, Heuristic<state> *backward, std::vector<state> &thePath) {
		if (!InitializeSearch(e, from, to, forward, backward, thePath)) return;
		while (!DoSingleSearchStep(thePath)) {}
	}

	virtual const char *GetName() { return "BiAStar"; }
	void ResetNodeCount() { nodesExpanded = nodesTouched = 0; }
	uint64_t GetNodesExpanded() const { return nodesExpanded; }
	uint64_t GetNodesTouched() const { return nodesTouched; }

	// Inspection (used by the trace tool). The node's .h field holds its priority (the f-value).
	int GetNumForwardItems() { return forwardQueue.size(); }
	const AStarOpenClosedData<state> &GetForwardItem(unsigned int i) { return forwardQueue.Lookat(i); }
	int GetNumBackwardItems() { return backwardQueue.size(); }
	const AStarOpenClosedData<state> &GetBackwardItem(unsigned int i) { return backwardQueue.Lookat(i); }

	bool InitializeSearch(environment *e, const state &from, const state &to,
						  Heuristic<state> *forward, Heuristic<state> *backward, std::vector<state> &thePath) {
		env = e; forwardHeuristic = forward; backwardHeuristic = backward;
		currentCost = DBL_MAX;
		forwardQueue.Reset(); backwardQueue.Reset();
		ResetNodeCount();
		thePath.resize(0);
		start = from; goal = to;
		if (start == goal) return false;
		forwardQueue.AddOpenNode(start, env->GetStateHash(start), 0, forwardHeuristic->HCost(start, goal));
		backwardQueue.AddOpenNode(goal, env->GetStateHash(goal), 0, backwardHeuristic->HCost(goal, start));
		expandForward = true;
		return true;
	}

	bool DoSingleSearchStep(std::vector<state> &thePath) {
		if ((forwardQueue.OpenSize() == 0 || backwardQueue.OpenSize() == 0) && currentCost == DBL_MAX)
			return true; // no solution
		if (currentCost <= getLowerBound()) {
			std::vector<state> pFor, pBack;
			ExtractPath(backwardQueue, middleNode, pBack);
			ExtractPath(forwardQueue, middleNode, pFor);
			std::reverse(pFor.begin(), pFor.end());
			thePath = pFor;
			thePath.insert(thePath.end(), pBack.begin() + 1, pBack.end());
			return true;
		}
		if (alternating) {
			if (expandForward) { Expand(forwardQueue, backwardQueue, forwardHeuristic, goal); expandForward = false; }
			else               { Expand(backwardQueue, forwardQueue, backwardHeuristic, start); expandForward = true; }
		} else { // cardinality (Pohl): expand the smaller frontier
			if (forwardQueue.OpenSize() > backwardQueue.OpenSize())
				Expand(backwardQueue, forwardQueue, backwardHeuristic, start);
			else
				Expand(forwardQueue, backwardQueue, forwardHeuristic, goal);
		}
		return false;
	}

private:
	// Lower bound on any remaining solution = max(fMin_F, fMin_B).
	double getLowerBound() {
		if (forwardQueue.OpenSize() == 0 || backwardQueue.OpenSize() == 0) return DBL_MAX;
		double fF = forwardQueue.Lookup(forwardQueue.Peek()).h;
		double fB = backwardQueue.Lookup(backwardQueue.Peek()).h;
		return std::max(fF, fB);
	}

	void ExtractPath(priorityQueue &q, const state &node, std::vector<state> &path) {
		uint64_t id; q.Lookup(env->GetStateHash(node), id);
		do { path.push_back(q.Lookup(id).data); id = q.Lookup(id).parentID; }
		while (q.Lookup(id).parentID != id);
		path.push_back(q.Lookup(id).data);
	}

	void Expand(priorityQueue &current, priorityQueue &opposite,
				Heuristic<state> *heuristic, const state &target) {
		uint64_t nextID;
		bool success = false;
		while (current.OpenSize() > 0) {           // lazy nipping: skip nodes closed on the other side
			nextID = current.Close();
			uint64_t revLoc;
			auto loc = opposite.Lookup(env->GetStateHash(current.Lookup(nextID).data), revLoc);
			if (loc != kClosedList) { success = true; break; }
		}
		if (!success) return;
		nodesExpanded++;

		env->GetSuccessors(current.Lookup(nextID).data, neighbors);
		for (auto &succ : neighbors) {
			nodesTouched++;
			uint64_t hash = env->GetStateHash(succ);
			uint64_t childID;
			auto loc = current.Lookup(hash, childID);
			double edge = env->GCost(current.Lookup(nextID).data, succ);
			double g = current.Lookup(nextID).g + edge;
			double h = heuristic->HCost(succ, target);
			double f = g + h;
			if (fgreatereq(f, currentCost)) continue;   // cannot improve on incumbent

			switch (loc) {
				case kClosedList: break; // consistent heuristic: closed = optimal
				case kOpenList: {
					auto &cd = current.Lookup(childID);
					if (fless(g, cd.g)) {
						cd.parentID = nextID; cd.g = g; cd.h = f; // .h stores priority (f)
						current.KeyChanged(childID);
						CheckSolution(opposite, hash, g, succ);
					}
					break;
				}
				case kNotFound: {
					current.AddOpenNode(succ, hash, g, f, nextID); // priority = f
					CheckSolution(opposite, hash, g, succ);
					break;
				}
			}
		}
	}

	void CheckSolution(priorityQueue &opposite, uint64_t hash, double g, const state &succ) {
		uint64_t revLoc;
		auto loc = opposite.Lookup(hash, revLoc);
		if (loc == kOpenList) {
			double total = g + opposite.Lookup(revLoc).g;
			if (fless(total, currentCost)) { currentCost = total; middleNode = succ; }
		}
	}

	priorityQueue forwardQueue, backwardQueue;
	state start, goal, middleNode;
	uint64_t nodesExpanded, nodesTouched;
	double currentCost;
	std::vector<state> neighbors;
	environment *env;
	Heuristic<state> *forwardHeuristic, *backwardHeuristic;
	bool alternating, expandForward;
};

#endif /* BiAStar_h */
