#ifndef _MWSS_SOLVER_HPP
#define _MWSS_SOLVER_HPP

#include "../graph.hpp"
#include "../branch-and-price/mp_solution.hpp"

#include <vector>

namespace sgcp {
    class MwssSolver {
        const Graph& o;
        const Graph& g;

        WeightMap make_weight_map(const MpSolution& mpsol) const;

    public:

        MwssSolver(const Graph& o, const Graph& g) : o{o}, g{g} {}
        std::vector<StableSet> solve(const MpSolution& mpsol) const;
    };
}

#endif