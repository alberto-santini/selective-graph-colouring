//
// Created by alberto on 11/03/17.
//

#ifndef SGCP_GRASP_HPP
#define SGCP_GRASP_HPP

#include <branch-and-price/column_pool.hpp>
#include "../graph.hpp"

namespace sgcp {
    class GRASPSolver {
        const Graph& g;

    public:
        GRASPSolver(const Graph& g) : g{g} {}
        ColumnPool solve() const;

    private:
        std::pair<Graph, WeightMap> reduce(const Graph& g, const WeightMap& wm, const std::set<uint32_t>& coloured_v) const;
        ColumnPool greedy_mwss_solve(WeightMap weights) const;
        WeightMap make_random_weight_map() const;
    };
}

#endif //SGCP_GRASP_HPP
