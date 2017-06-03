#include "campelo_mip_solver.hpp"
#include "../utils/console_colour.hpp"
#include "../utils/dbg_output.hpp"

#include <sstream>
#include <iostream>

namespace sgcp {
    boost::optional<StableSetCollection> CampeloMipSolver::solve() const {
        DEBUG_ONLY(using namespace Console;)

        IloEnv env;
        IloModel model(env);

        IloArray<IloNumVarArray> x(env, g.n_vertices);
        IloRangeArray colour_all_partitions(env, g.n_partitions);
        IloRangeArray is_stable_set(env);
        IloRangeArray turn_on(env);

        std::stringstream name;

        for(auto u = 0u; u < g.n_vertices; u++) {
            x[u] = IloNumVarArray(env, g.n_vertices);

            for(auto v = 0u; v < g.n_vertices; v++) {
                name << "x_" << u << "_" << v;
                x[u][v] = IloNumVar(env, 0, 1, IloNumVar::Bool, name.str().c_str());
                name.str(""); name.clear();
            }

            model.add(x[u]);
        }

        IloExpr expr(env);

        for(auto k = 0u; k < g.n_partitions; k++) {
            for(auto u : g.p[k]) {
                for(auto v : g.anti_neighbourhood_including_itself_of(u)) {
                    expr += x[v][u];
                }
            }

            name << "cap_" << k;
            colour_all_partitions[k] = IloRange(env, 1, expr, IloInfinity, name.str().c_str());
            name.str(""); name.clear(); expr.clear();
        }

        model.add(colour_all_partitions);

        for(auto u = 0u; u < g.n_vertices; u++) {
            for(auto v : g.anti_neighbourhood_of(u)) {
                for(auto w : g.anti_neighbourhood_of(u)) {
                    if(!g.connected(v, w)) { continue; }

                    name << "iss_" << u << "_" << v << "_" << w;
                    is_stable_set.add(IloRange(env, -IloInfinity, x[u][v] + x[u][w] - x[u][u], 0, name.str().c_str()));
                    name.str(""); name.clear();
                }

                name << "to_" << u << "_" << v;
                turn_on.add(IloRange(env, -IloInfinity, x[u][v] - x[u][u], 0, name.str().c_str()));
                name.str(""); name.clear();
            }
        }

        model.add(is_stable_set);
        model.add(turn_on);

        // Break symmetry by zero-ing half matrix
        for(auto u = 0u; u < g.n_vertices; u++) {
            for(auto v = u + 1; v < g.n_vertices; v++) {
                x[u][v].setUB(0);
            }
        }

        for(auto u = 0u; u < g.n_vertices; u++) {
            expr += x[u][u];
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
            DEBUG_ONLY(std::cout << std::endl << Colour::Yellow << "Campelo MIP Solution: " << cplex.getObjValue() << Colour::Default << std::endl;)
            auto ss = make_stable_sets(cplex, x);
            env.end();
            return ss;
        } else {
          std::cerr << "Campelo MIP Cplex error!" << std::endl;
          std::cerr << "\tStatus: " << cplex.getStatus() << std::endl;
          std::cerr << "\tSolver status: " << cplex.getCplexStatus() << std::endl;
          env.end();
          return boost::none;
        }
    }

    StableSetCollection CampeloMipSolver::make_stable_sets(const IloCplex& cplex, const IloArray<IloNumVarArray>& x) const {
        StableSetCollection sol;

        for(auto u = 0u; u < g.n_vertices; u++) {
            if(cplex.getValue(x[u][u]) > 0.5) {
                VertexIdSet s;
                for(auto v = 0u; v <= u; v++) { if(cplex.getValue(x[u][v]) > 0.5) { s.insert(v); } }
                sol.emplace_back(g, s);
            }
        }

        return sol;
    }
}