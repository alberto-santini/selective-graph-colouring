#include "dumb_heuristic.hpp"

namespace sgcp {
    StableSetCollection DumbHeuristicSolver::solve() const {
        StableSetCollection sol;

        for(auto k = 0u; k < g.n_partitions; k++) {
            auto it = g.p[k].begin();
            auto v = g.vertex_by_id(*it);
            assert(v);

            VertexIdSet s;

            for(auto orig_id : g.g[*v].represented_vertices) {
                s.insert(orig_id);
            }

            sol.emplace_back(g, s);
        }

        return sol;
    }
}