#ifndef _MP_SOLVER_HPP
#define _MP_SOLVER_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"
#include "mp_solution.hpp"
#include "column_pool.hpp"

#include <vector>
#include <boost/optional.hpp>

#ifndef IL_STD
    #define IL_STD
#endif

#include <cstring>
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN

namespace sgcp {
    class MpSolver {
        const Graph& g;
        const ColumnPool& c;
        
        mutable IloEnv env;
        mutable IloModel model;
        mutable IloCplex cplex;
        mutable IloNumVarArray x;
        mutable IloRangeArray colour;
        mutable IloObjective obj;
        
        bool lp;

        static constexpr float eps = 1e-6;

        MpSolution make_solution() const;
        void create_model() const;
        
        boost::optional<MpSolution> solve(const std::vector<uint32_t>& forbidden_columns, bool skip_forbidden_columns_check, bool first_node_tilim) const;

    public:
        MpSolver(const Graph& g, const ColumnPool& c, bool lp) : g{g}, c{c}, lp{lp} { create_model(); }
        
        ~MpSolver() { env.end(); }
        
        boost::optional<MpSolution> solve_with_forbidden_check(const std::vector<uint32_t>& forbidden_columns) const;
        boost::optional<MpSolution> solve_without_forbidden_check(const std::vector<uint32_t>& forbidden_columns) const;
        boost::optional<MpSolution> solve_with_first_node_tilim(const std::vector<uint32_t>& forbidden_columns) const;
        void add_column(const StableSet& col);
        void add_mipstart(const std::vector<uint32_t>& mipstart_columns) const;
    };
}

#endif