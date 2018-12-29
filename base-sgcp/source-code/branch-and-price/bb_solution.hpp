#ifndef _BB_SOLUTION_HPP
#define _BB_SOLUTION_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"
#include "column_pool.hpp"
#include "mp_solution.hpp"

#include <cmath>
#include <vector>

namespace sgcp {
    struct BBSolution {
        float ub;
        float lb;
        ActiveColumnsWithCoeff integer_solution_columns;
        ActiveColumnsWithCoeff lp_solution_columns;
        std::vector<uint32_t> forbidden_columns;
        std::shared_ptr<const Graph> g;
        bool timeout;

        static constexpr float eps = 1e-6;

        BBSolution( float ub,
                    float lb,
                    ActiveColumnsWithCoeff integer_solution_columns,
                    ActiveColumnsWithCoeff lp_solution_columns,
                    std::vector<uint32_t> forbidden_columns,
                    std::shared_ptr<const Graph> g,
                    bool timeout = false) :
                    ub{ub},
                    lb{lb},
                    integer_solution_columns{integer_solution_columns},
                    lp_solution_columns{lp_solution_columns},
                    forbidden_columns{forbidden_columns},
                    g{g},
                    timeout{timeout}
        {
            assert(std::none_of(
                lp_solution_columns.begin(),
                lp_solution_columns.end(),
                [&] (auto colval) -> bool {
                    return std::find(forbidden_columns.begin(), forbidden_columns.end(), colval.first) != forbidden_columns.end();
                }
            ));
        }

        bool node_solved_to_optimality() const { return ub - std::ceil(lb) < eps; }
    };
}

#endif