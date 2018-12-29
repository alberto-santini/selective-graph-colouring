#include "mwss_solver.hpp"
#include "sewell_mwss_solver.hpp"

namespace sgcp {
    WeightMap MwssSolver::make_weight_map(const MpSolution& mpsol) const {
        WeightMap w;

        for(auto it = vertices(g.g); it.first != it.second; ++it.first) {
            uint32_t key = g.g[*it.first].id;
            float value = 0;

            for(uint32_t rid : g.g[*it.first].represented_vertices) {
                uint32_t k = o.partition_for(rid);
                value += mpsol.duals.at(k);
            }

            w[key] = value;
        }

        return w;
    }

    std::vector<StableSet> MwssSolver::solve(const MpSolution& mpsol) const {
        auto solutions = std::vector<StableSet>{};
        auto w = make_weight_map(mpsol);

        auto sew_solv = SewellMwssSolver{o, g, w};
        auto solution = sew_solv.solve();

        if(solution) {
          assert(solution->is_valid(true));
          solutions.push_back(*solution);
        }

        return solutions;
    }
}
