#ifndef _DUMB_HEURISTIC_HPP
#define _DUMB_HEURISTIC_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"

namespace sgcp {
    struct DumbHeuristicSolver {
        const Graph& g;
        
        DumbHeuristicSolver(const Graph& g) : g{g} {}
        
        // Returns the dumbest possible colouring of the graph:
        // it takes one vertex from each partition and puts each
        // selected vertex in a stable set on his own.
        StableSetCollection solve() const;
    };
}

#endif