#ifndef _INITIAL_SOLUTIONS_GENERATOR_HPP
#define _INITIAL_SOLUTIONS_GENERATOR_HPP

#include "../graph.hpp"
#include "../solver_stats.hpp"
#include "column_pool.hpp"

namespace sgcp {
    struct InitialSolution {
        ColumnPool columns;
        std::vector<uint32_t> feasible_solution_ids;
        float time_spent;
    };

    class InitialSolutionsGenerator {
        const Graph& g;
        SolverStats& stats;

        void add_unique(ColumnPool& pool, const ColumnPool& add) const;

    public:
        InitialSolutionsGenerator(const Graph& g, SolverStats& stats) : g{g}, stats{stats} {}

        InitialSolution generate_from_existing(ColumnPool& start_solution);
        InitialSolution generate();
    };
}

#endif