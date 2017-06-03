#include "decomposition_model_helper.hpp"

#include <fstream>
#include <sstream>
#include <iterator>
#include <numeric>

namespace sgcp {
    PartitionsCliqueVec DecompositionModelHelper::all_partitions_in_pair_clique() const {
        PartitionsCliqueVec couples;

        for(auto k1 = 0u; k1 < g.n_partitions; k1++) {
            for(auto k2 = k1 + 1; k2 < g.n_partitions; k2++) {
                bool in_clique = true;

                for(auto v1_id : g.p[k1]) {
                    auto v1 = g.vertex_by_id(v1_id);
                    assert(v1);

                    for(auto v2_id : g.p[k2]) {
                        auto v2 = g.vertex_by_id(v2_id);
                        assert(v2);

                        if(!edge(*v1, *v2, g.g).second) {
                            in_clique = false;
                            break;
                        }
                    }

                    if(!in_clique) { break; }
                }

                if(in_clique) { couples.push_back(std::make_pair(k1, k2)); }
            }
        }
        
        std::cout << "Added " << couples.size() << " clique cuts" << std::endl;

        return couples;
    }
    
    void DecompositionModelHelper::build_vars_and_obj(IloArray<IloNumVarArray>& x, IloNumVarArray& z) const {
        std::stringstream ss;
        IloExpr expr(env);

        for(auto k = 0u; k < g.n_partitions; k++) {
            x[k] = IloNumVarArray(env, std::min(k+1, ub));

            for(auto c = 0u; c < std::min(k+1, ub); c++) {
                ss << "x_" << k << "_" << c;
                x[k][c] = IloNumVar(env, 0, 1, IloNumVar::Int, ss.str().c_str());
                ss.str("");
            }
        }

        for(auto c = 0u; c < ub; c++) {
            ss << "z_" << c;
            z[c] = IloNumVar(env, 0, 1, IloNumVar::Int, ss.str().c_str());
            expr += z[c];
            ss.str("");
        }

        IloObjective obj(env, expr, IloObjective::Minimize, "objective");
        model.add(obj);
        expr.end();
    }

    void DecompositionModelHelper::build_constraints(IloRangeArray& link, IloRangeArray& col, IloRangeArray& clique, IloArray<IloNumVarArray>& x, IloNumVarArray& z) const {
        std::stringstream ss;
        IloExpr expr(env);

        for(auto c = 0u; c < ub; c++) {
            ss << "link_" << c;
            for(auto k = c; k < g.n_partitions; k++) {
                expr += x[k][c];
            }
            expr -= (int)(g.n_partitions - c) * z[c];

            link[c] = IloRange(env, -IloInfinity, expr, 0, ss.str().c_str());
            ss.str(""); expr.clear();
        }

        for(auto k = 0u; k < g.n_partitions; k++) {
            ss << "col_" << k;
            for(auto c = 0u; c < std::min(k+1, ub); c++) {
                expr += x[k][c];
            }

            col[k] = IloRange(env, 1, expr, IloInfinity, ss.str().c_str());
            ss.str(""); expr.clear();
        }

        auto clique_partitions = all_partitions_in_pair_clique();

        uint32_t clique_cstr_n = 0;
        for(const auto& pr : clique_partitions) {
            auto i1 = pr.first, i2 = pr.second;
            for(auto c = 0u; c < std::min(std::min(i1+1, i2+1), ub); c++) {
                ss << "clq_" << clique_cstr_n++;
                clique.add(IloRange(env, -IloInfinity, x[i1][c] + x[i2][c], 1, ss.str().c_str()));
                ss.str("");
            }
        }

        model.add(link);
        model.add(col);
        model.add(clique);
        expr.end();
    }

    void DecompositionModelHelper::try_cplex_solve(IloCplex& cplex) const {
        bool solved = false;

        try {
            solved = cplex.solve();
        } catch(const IloException& e) {
            std::cerr << "CPLEX Raised an exception:" << std::endl;
            std::cerr << e << std::endl;
            env.end();
            throw;
        }

        if(!solved) {
            std::cerr << "Status: " << cplex.getStatus() << std::endl;
            std::cerr << "Solver status: " << cplex.getCplexStatus() << std::endl;
            env.end();
            throw std::runtime_error("First-stage problem infeasible?!");
        }
    }

    PartitionsVec DecompositionModelHelper::get_partitions(IloCplex& cplex, IloArray<IloNumVarArray>& x) const {
        auto cp = PartitionsVec(ub, PartitionsIdVec());
        static constexpr float eps = 1e-6;

        for(auto k = 0u; k < g.n_partitions; k++) {
            for(auto c = 0u; c < std::min(k+1, ub); c++) {
                if(cplex.getValue(x[k][c]) > eps) {
                    cp[c].push_back(k);
                }
            }
        }

        cp.erase(std::remove_if(cp.begin(), cp.end(), [&] (const auto& p) { return p.empty(); }), cp.end());
        return cp;
    }

    std::vector<IloRange> DecompositionModelHelper::generate_constraint_for(IloArray<IloNumVarArray>& x, const PartitionsIdVec& p) const {
        std::vector<IloRange> csts;
        auto me = *std::min_element(p.begin(), p.end());
        IloExpr expr(env);
        
        for(auto c = 0u; c < std::min(me + 1, ub); c++) {
            for(auto k : p) { expr += x[k][c]; }

            csts.push_back(IloRange(env, -IloInfinity, expr, (int)(p.size() - 1)));
            expr.clear();
        }

        expr.end();
        return csts;
    }
    
    void DecompositionModelHelper::add_all_3incompatible_cuts(IloArray<IloNumVarArray>& x) {
        uint32_t cuts_n = 0u;
        for(auto k1 = 0u; k1 < g.p.size(); k1++) {
            for(auto k2 = k1 + 1; k2 < g.p.size(); k2++) {
                for(auto k3 = k2 + 1; k3 < g.p.size(); k3++) {
                    PartitionsIdVec v = {k1, k2, k3};
                    PartitionsIdSet s(v.begin(), v.end());
                    if(!gh.can_be_coloured_the_same(s)) {
                        auto c = generate_constraint_for(x, v);
                        for(auto cst : c) { model.add(cst); }
                        cuts_n += c.size();
                    }
                }
            }
        }
        std::cout << "Added " << cuts_n << " 3-cuts" << std::endl;
    }
    
    void DecompositionModelHelper::add_best_3incompatible_cuts(IloArray<IloNumVarArray>& x) {
        uint32_t cuts_n = 0u;
        PartitionsIdVec sorted_partitions(g.n_partitions);
        std::iota(sorted_partitions.begin(), sorted_partitions.end(), 0);
        
        std::sort(
            sorted_partitions.begin(),
            sorted_partitions.end(),
            [this] (auto p1, auto p2) { return this->gh.partition_external_degree(p1) < this->gh.partition_external_degree(p2); }
        );
        
        for(auto k1 = 0u; k1 < g.n_partitions; k1++) {
            for(auto k2 = k1 + 1; k2 < g.n_partitions; k2++) {
                for(auto k3 = k2 + 1; k3 < g.n_partitions; k3++) {
                    PartitionsIdVec v = {k1, k2, k3};
                    PartitionsIdSet s(v.begin(), v.end());
                    if(!gh.can_be_coloured_the_same(s)) {
                        auto c = generate_constraint_for(x, v);
                        for(auto cst : c) { model.add(cst); }
                        cuts_n += c.size();
                    }
                    if(cuts_n > g.params.decomposition_max_added_cuts_when_caching) { return; }
                }
            }
        }
    }
    
    void DecompositionModelHelper::try_initial_solution_from_file(std::string filename, IloCplex& cplex, IloRangeArray& link, IloRangeArray& col, IloRangeArray& clique, IloArray<IloNumVarArray>& x, IloNumVarArray& z) const {
        std::ifstream f(filename);
        std::string line;
        
        PartitionsVec read_partitions;
        
        while(std::getline(f, line)) {
            std::istringstream ss(line);
            read_partitions.push_back(PartitionsIdVec{});
            std::copy(std::istream_iterator<uint32_t>(ss), std::istream_iterator<uint32_t>(), std::back_inserter(read_partitions.back()));
        }
        
        std::sort(read_partitions.begin(), read_partitions.end(), [] (const auto& r1, const auto& r2) { return r1[0] < r2[0]; });
        for(auto c = 0u; c < read_partitions.size(); c++) {
            std::cout << "Colour " << c << ": " << read_partitions[c] << std::endl;
        }
        
        IloNumVarArray initial_vars(env);
        IloNumArray initial_vals(env);
        
        for(auto k = 0u; k < g.n_partitions; k++) {
            for(auto c = 0u; c < std::min(k+1, ub); c++) {
                initial_vars.add(x[k][c]);
                initial_vals.add(
                    std::find(read_partitions[c].begin(), read_partitions[c].end(), k) != read_partitions[c].end() ?
                    1 : 0
                );
            }
        }
        
        for(auto c = 0u; c < ub; ++c) {
            initial_vars.add(z[c]);
            initial_vals.add(c < read_partitions.size() ? 1 : 0);
        }
        
        cplex.addMIPStart(initial_vars, initial_vals);
        
        IloConstraintArray constraints(env);
        constraints.add(link);
        constraints.add(col);
        constraints.add(clique);
        
        IloNumArray preferences(env);
        for(auto i = 0; i < constraints.getSize(); i++) { preferences.add(1.0); }
        
        if(cplex.refineMIPStartConflict(0, constraints, preferences)) {
            IloCplex::ConflictStatusArray conflict = cplex.getConflict(constraints);
            env.getImpl()->useDetailedDisplay(IloTrue);
            for(auto i = 0; i < constraints.getSize(); i++) {
                if(conflict[i] == IloCplex::ConflictMember) {
                    std::cerr << "Conflict: " << constraints[i] << std::endl;
                }
                if(conflict[i] == IloCplex::ConflictPossibleMember) {
                    std::cerr << "Possible conflict: " << constraints[i] << std::endl;
                }
            }
        }
        
        initial_vars.end();
        initial_vals.end();
    }
    
    void DecompositionModelHelper::set_initial_solution(const MpSolution& init, IloCplex& cplex, IloRangeArray& link, IloRangeArray& col, IloRangeArray& clique, IloArray<IloNumVarArray>& x, IloNumVarArray& z) const {
        IloNumVarArray initial_vars(env);
        IloNumArray initial_vals(env);
        
        PartitionsVec init_p;
        for(const auto& set_val : init.columns) {
            init_p.push_back(PartitionsIdVec{});
            for(auto v_id : set_val.first.get_set()) {
                init_p.back().push_back(g.partition_for(v_id));
            }
        }
        std::sort(init_p.begin(), init_p.end(), [] (const auto& r1, const auto& r2) { return r1[0] < r2[0]; });
        
        for(auto k = 0u; k < g.n_partitions; k++) {
            for(auto c = 0u; c < std::min(k+1, ub); c++) {
                initial_vars.add(x[k][c]);
                initial_vals.add(
                    std::find(init_p[c].begin(), init_p[c].end(), k) != init_p[c].end() ?
                    1 : 0
                );
            }
        }
        
        for(auto c = 0u; c < ub; ++c) {
            initial_vars.add(z[c]);
            initial_vals.add(c < init_p.size() ? 1 : 0);
        }
        
        cplex.addMIPStart(initial_vars, initial_vals);
        
        initial_vars.end();
        initial_vals.end();        
    }
}