#include "compact_mip_solver.hpp"
#include "../utils/console_colour.hpp"
#include "../utils/dbg_output.hpp"

#include <sstream>
#include <iostream>

namespace sgcp {
    boost::optional<StableSetCollection> CompactMipSolver::solve() const {
        DEBUG_ONLY(using namespace Console;)

        IloEnv env;
        IloModel model(env);

        IloArray<IloNumVarArray> x(env, g.n_vertices);
        IloNumVarArray y(env, g.n_partitions);
        IloRangeArray colour_all_partitions(env, g.n_partitions);
        IloRangeArray is_stable_set(env);

        std::stringstream name;

        for(auto u = 0u; u < g.n_vertices; u++) {
            x[u] = IloNumVarArray(env, g.n_partitions);

            for(auto k = 0u; k < g.n_partitions; k++) {
                name << "x_" << u << "_" << k;
                x[u][k] = IloNumVar(env, 0, 1, IloNumVar::Bool, name.str().c_str());
                name.str(""); name.clear();
            }

            model.add(x[u]);
        }

        for(auto k = 0u; k < g.n_partitions; k++) {
            name << "y_" << k;
            y[k] = IloNumVar(env, 0, 1, IloNumVar::Bool, name.str().c_str());
            name.str(""); name.clear();
        }

        model.add(y);

        IloExpr expr(env);

        for(auto p = 0u; p < g.n_partitions; p++) {
            for(auto u : g.p[p]) {
                for(auto k = 0u; k < g.n_partitions; k++) {
                    expr += x[u][k];
                }
            }

            name << "cap_" << p;
            colour_all_partitions[p] = IloRange(env, 1, expr, IloInfinity, name.str().c_str());
            name.str(""); name.clear(); expr.clear();
        }

        model.add(colour_all_partitions);

        for(auto k = 0u; k < g.n_partitions; k++) {
            for(auto u = 0u; u < g.n_vertices; u++) {
                for(auto v = 0u; v < g.n_vertices; v++) {
                    if(!g.connected(u, v)) { continue; }

                    name << "iss_" << k << "_" << u << "_" << v;
                    is_stable_set.add(IloRange(env, -IloInfinity, x[u][k] + x[v][k] - y[k], 0, name.str().c_str()));
                    name.str(""); name.clear();
                }
            }
        }

        model.add(is_stable_set);

        for(auto k = 0u; k < g.n_partitions; k++) {
            expr += y[k];
        }

        IloObjective obj(env, expr, IloObjective::Minimize);

        model.add(obj);
        expr.end();

        IloCplex cplex(model);

        cplex.setParam(IloCplex::TiLim, g.params.time_limit);
        cplex.setParam(IloCplex::Threads, g.params.cplex_threads);
        cplex.setParam(IloCplex::Param::Parallel, IloCplex::Opportunistic);

        bool solved = false;

        try {
            solved = cplex.solve();
        } catch(const IloException& e) {
            std::cerr << "CPLEX Raised an exception:" << std::endl;
            std::cerr << e << std::endl;
            env.end();
            throw;
        }

        if(solved) {
            DEBUG_ONLY(std::cout << std::endl << Colour::Yellow << "Compact MIP Solution: " << cplex.getObjValue() << Colour::Default << std::endl;)
            auto ss = make_stable_sets(cplex, x, y);
            env.end();
            return ss;
        } else {
          std::cerr << "Compact MIP Cplex error!" << std::endl;
          std::cerr << "\tStatus: " << cplex.getStatus() << std::endl;
          std::cerr << "\tSolver status: " << cplex.getCplexStatus() << std::endl;
          env.end();
          return boost::none;
        }
    }

    StableSetCollection CompactMipSolver::make_stable_sets(const IloCplex& cplex, const IloArray<IloNumVarArray>& x, const IloNumVarArray& y) const {
        StableSetCollection sol;

        for(auto k = 0u; k < g.n_partitions; k++) {
            if(cplex.getValue(y[k]) > 0.5) {
                VertexIdSet s;
                for(auto u = 0u; u < g.n_vertices; u++) { if(cplex.getValue(x[u][k]) > 0.5) { s.insert(u); } }
                sol.emplace_back(g, s);
            }
        }

        return sol;
    }
}