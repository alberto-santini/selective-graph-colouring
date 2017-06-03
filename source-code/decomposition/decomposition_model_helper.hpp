#ifndef _DECOMPOSITION_MODEL_HELPER
#define _DECOMPOSITION_MODEL_HELPER

#include "../graph.hpp"
#include "../branch-and-price/mp_solution.hpp"
#include "decomposition_helper.hpp"
#include "decomposition_graph_helper.hpp"

#ifndef IL_STD
    #define IL_STD
#endif

#include <cstring>
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN

namespace sgcp {
    struct DecompositionModelHelper {
        const Graph& g;
        IloEnv& env;
        IloModel& model;
        const DecompositionGraphHelper& gh;
        uint32_t ub;
        
        PartitionsCliqueVec all_partitions_in_pair_clique() const;
        void build_vars_and_obj(IloArray<IloNumVarArray>& x, IloNumVarArray& z) const;
        void build_constraints(IloRangeArray& link, IloRangeArray& col, IloRangeArray& clique, IloArray<IloNumVarArray>& x, IloNumVarArray& z) const;
        void try_cplex_solve(IloCplex& cplex) const;
        PartitionsVec get_partitions(IloCplex& cplex, IloArray<IloNumVarArray>& x) const;
        std::vector<IloRange> generate_constraint_for(IloArray<IloNumVarArray>& x, const PartitionsIdVec& p) const;
        void add_all_3incompatible_cuts(IloArray<IloNumVarArray>& x);
        void add_best_3incompatible_cuts(IloArray<IloNumVarArray>& x);
        void set_initial_solution(const MpSolution& init, IloCplex& cplex, IloRangeArray& link, IloRangeArray& col, IloRangeArray& clique, IloArray<IloNumVarArray>& x, IloNumVarArray& z) const;
        void try_initial_solution_from_file(std::string filename, IloCplex& cplex, IloRangeArray& link, IloRangeArray& col, IloRangeArray& clique, IloArray<IloNumVarArray>& x, IloNumVarArray& z) const;
        
        DecompositionModelHelper(const Graph& g, IloEnv& env, IloModel& model, const DecompositionGraphHelper& gh, uint32_t ub) : g{g}, env{env}, model{model}, gh{gh}, ub{ub} {}
    };
}

#endif