#ifndef _GREEDY_HEURISTIC_HPP
#define _GREEDY_HEURISTIC_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"

namespace sgcp {
    struct GreedyHeuristicSolver {
        const Graph& g;

        GreedyHeuristicSolver(const Graph& g) : g{g} {}

		StableSetCollection solve_simple() const { return solve(false); }
        StableSetCollection solve_improved() const { return solve(true); }
        StableSetCollection solve() const;

    private:
    		// Colours one vertex in one partition, then greedily tries
    		// to augment the stable set, until it can't anymore and
    		// create a new stable set.
        StableSetCollection solve(bool improved) const;
    };
}

#endif
