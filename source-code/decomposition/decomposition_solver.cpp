#include "decomposition_solver.hpp"
#include "../branch-and-price/initial_solutions_generator.hpp"
#include "../branch-and-price/mp_solver.hpp"
#include "../utils/console_colour.hpp"

#include <iostream>
#include <deque>

namespace sgcp {
    DecompositionSolver::DecompositionSolver(const Graph& g) : g{g}, gh{g} {
        if(g.params.decomposition_3cuts_strategy == "cache") {
            inc_strategy = DecompositionSolver::IncompatibleThreeCutsStrategy::AddWhenViolated;
        } else if(g.params.decomposition_3cuts_strategy == "add_all") {
            inc_strategy = DecompositionSolver::IncompatibleThreeCutsStrategy::AddAllOffline;
        } else {
            throw "Three-cuts strategy not recognised!";
        }
    }

    uint32_t DecompositionSolver::get_upper_bound() const {
        SolverStats stats{g.params};

        stats.instance = g.data_filename;
        stats.n_vertices = g.n_vertices;
        stats.n_edges = g.n_edges;
        stats.n_partitions = g.n_partitions;

        InitialSolutionsGenerator is{g, stats};

        auto columns = is.generate();
        auto fcolumns = std::vector<uint32_t>{};
        MpSolver mp{g, columns.columns, false};
        mp.add_mipstart(columns.feasible_solution_ids);
        auto initial_solution = mp.solve_without_forbidden_check(fcolumns);

        if(initial_solution) {
            return static_cast<uint32_t>(initial_solution->obj_value);
        } else {
            return g.n_partitions;
        }
    }
    
    PartitionsVec DecompositionSolver::cached_3incompatible_cuts(const PartitionsIdVec& partitions) const {
        PartitionsVec ps;
        
        for(const auto& cp : inc_cache) {
            if(std::all_of(
                cp.begin(), cp.end(),
                [&partitions] (auto p) {
                    return std::find(partitions.begin(), partitions.end(), p) != partitions.end();
                }
            )) { ps.emplace_back(cp.begin(), cp.end()); }
        }
        
        return ps;
    }

    PartitionsVec DecompositionSolver::uncolourable_subpartitions(PartitionsIdVec partitions) const {
        PartitionsVec ps;
        
        if(partitions.size() < 2u) { return ps; }
        
        if(inc_strategy == IncompatibleThreeCutsStrategy::AddWhenViolated) { ps = cached_3incompatible_cuts(partitions); }
        
        if(!ps.empty()) { return ps; }

        auto sorted = partitions;
        std::sort(sorted.begin(), sorted.end(), [this] (auto k1, auto k2) { return this->gh.partition_external_degree(k1) > this->gh.partition_external_degree(k2); });
        assert(sorted.size() < 2u || gh.partition_external_degree(sorted[0]) >= gh.partition_external_degree(sorted[1]));

        auto idx = std::max(1, static_cast<int>(sorted.size()) - static_cast<int>(g.params.decomposition_lifting_coeff));
        
        PartitionsIdSet heur_partitions(sorted.begin(), sorted.begin() + 3);
        for(auto i = 4; i < idx; i++) {
            if(!gh.can_be_coloured_the_same(heur_partitions)) {
                ps.emplace_back(heur_partitions.begin(), heur_partitions.end());
                return ps;
            } else {
                heur_partitions.insert(sorted[i]);
            }
        }
            
        sorted.erase(sorted.begin() + idx, sorted.end());
            
        std::deque<PartitionsIdSet> pqueue;
        pqueue.emplace_back(sorted.begin(), sorted.end());
        
        while(!pqueue.empty()) {
            auto p = pqueue.front();
            pqueue.pop_front();
            
            if(gh.can_be_coloured_the_same(p)) {
                auto m = *std::max_element(p.begin(), p.end());
                
                for(auto k : partitions) {
                    if(k > m) {
                        auto new_p = p;
                        new_p.insert(k);
                        
                        if(std::find(pqueue.begin(), pqueue.end(), new_p) == pqueue.end()) { pqueue.push_back(new_p); }
                    }
                }
            } else {
                ps.emplace_back(p.begin(), p.end());
            }
        }

        return ps;
    }
    
    void DecompositionSolver::cache_all_3incompatible_cuts() {
        uint32_t n_cached = 0u;
        
        for(auto k1 = 0u; k1 < g.n_partitions; k1++) {
            for(auto k2 = k1 + 1; k2 < g.n_partitions; k2++) {
                for(auto k3 = k2 + 1; k3 < g.n_partitions; k3++) {
                    PartitionsIdSet tmp; tmp.insert(k1); tmp.insert(k2); tmp.insert(k3);
                    if(!gh.can_be_coloured_the_same(tmp)) {
                        inc_cache.insert(tmp);
                        n_cached++;
                    }
                }
            }
        }
        
        std::cout << "Cached " << n_cached << " 3-cuts" << std::endl;
    }

    void DecompositionSolver::solve() {
        using namespace Console;

        ub = get_upper_bound();

        IloEnv env;
        IloModel model(env);
        
        DecompositionModelHelper mh{g, env, model, gh, ub};
        
        IloArray<IloNumVarArray> x(env, g.n_partitions);
        IloNumVarArray z(env, ub);
        
        mh.build_vars_and_obj(x, z);
        
        IloRangeArray link(env, ub), col(env, g.n_partitions), clique(env);
        
        mh.build_constraints(link, col, clique, x, z);
        
        if(inc_strategy == IncompatibleThreeCutsStrategy::AddAllOffline) { mh.add_all_3incompatible_cuts(x); }
        if(inc_strategy == IncompatibleThreeCutsStrategy::AddWhenViolated) {
            cache_all_3incompatible_cuts();
            mh.add_best_3incompatible_cuts(x);
        }

        IloCplex cplex(model);

        cplex.setParam(IloCplex::Param::TimeLimit, g.params.decomposition_first_stage_time_limit);
        cplex.setParam(IloCplex::Param::Threads, g.params.cplex_threads);
        cplex.setParam(IloCplex::Param::Parallel, IloCplex::Opportunistic);
        cplex.setOut(env.getNullStream());
        cplex.setWarning(env.getNullStream());
        
        if(initial_solution) {
            mh.set_initial_solution(*initial_solution, cplex, link, col, clique, x, z);
        }

        while(true) {
            mh.try_cplex_solve(cplex);
            auto partitions = mh.get_partitions(cplex, x);

            std::cout << Colour::Magenta << "First-stage solution: " << partitions.size() << Colour::Default << std::endl;

            if(partitions.size() == ub) {
                std::cout << std::endl << Colour::Yellow << "Optimal solution found: " << partitions.size() << Colour::Default << std::endl;
                break;
            }

            std::vector<IloRange> new_constraints;
            for(const auto& p : partitions) {
                std::cout << "Partitions " << p;
                
                auto incompatible_partition_sets = uncolourable_subpartitions(p);

                if(incompatible_partition_sets.empty()) {
                    std::cout << Colour::Green << "can be coloured with the same colour" << Colour::Default << std::endl;
                } else {
                    std::cout << Colour::Red << "cannot be coloured with the same colour" << Colour::Default << std::endl;
                    for(const auto& ip : incompatible_partition_sets) {
                        std::cout << "\tIncompatible: " << ip << std::endl;
                        auto c = mh.generate_constraint_for(x, ip);
                        new_constraints.insert(new_constraints.end(), c.begin(), c.end());
                    }
                }
            }

            if(new_constraints.empty()) {
                std::cout << std::endl << Colour::Yellow << "Optimal solution found: " << partitions.size() << Colour::Default << std::endl;
                break;
            } else {
                std::cout << Colour::Yellow << "First-stage solution not feasible. Adding " << new_constraints.size() << " cutting planes" << Colour::Default << std::endl << std::endl;
                for(auto& cst : new_constraints) { model.add(cst); }
            }
        }
    }
}