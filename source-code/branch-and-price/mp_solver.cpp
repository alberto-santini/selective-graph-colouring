#include "mp_solver.hpp"
#include "../utils/console_colour.hpp"
#include "../utils/dbg_output.hpp"

#include <stdexcept>

namespace sgcp {
    void MpSolver::create_model() const {
        DEBUG_ONLY(using namespace Console;)

        env = IloEnv();
        model = IloModel(env);

        x = IloNumVarArray(env);
        colour = IloRangeArray(env, g.n_partitions);
        obj = IloMinimize(env);

        std::stringstream name;

        for(auto k = 0u; k < g.n_partitions; k++) {
            name << "colour_" << k;
            colour[k] = IloRange(env, 1, IloInfinity, name.str().c_str());
            name.str("");name.clear();
        }

        for(auto cid = 0u; cid < c.size(); cid++) {
            // Give a high price to the dummy column. For example,
            // 2 * number of vertices is more costly than any other
            // feasible solution (which uses at most 1 colour for
            // each partition).
            auto col_price = c.at(cid).dummy ? (2 * g.n_vertices) : 1;
            IloNumColumn col = obj(col_price);

            for(auto k = 0u; k < g.n_partitions; ++k) {
                if(c.at(cid).intersects(k)) {
                    col += colour[k](1);
                }
            }

            name << "x_" << cid;
            x.add(IloNumVar(col, 0, (lp ? IloInfinity : 1), (lp ? IloNumVar::Float : IloNumVar::Bool), name.str().c_str()));
            name.str(""); name.clear();
        }

        model.add(x);
        model.add(colour);
        model.add(obj);

        cplex = IloCplex(model);

        cplex.setOut(env.getNullStream());

        if(lp) {
            cplex.setParam(IloCplex::TiLim, g.params.mp_time_limit);
        } else {
            cplex.setParam(IloCplex::TiLim, g.params.mip_heur_time_limit);
        }
        cplex.setParam(IloCplex::Threads, g.params.cplex_threads);
        cplex.setParam(IloCplex::Param::Parallel, IloCplex::Opportunistic);
        cplex.setParam(IloCplex::RootAlg, IloCplex::Concurrent);
    }

    MpSolution MpSolver::make_solution() const {
        std::map<StableSet, float> columns;
        std::vector<float> duals(g.n_partitions, 0.0);

        for(auto cid = 0u; cid < c.size(); cid++) {
            auto val = cplex.getValue(x[cid]);
            if(val > eps) { columns[c.at(cid)] = val; }
        }

        if(lp) { for(auto k = 0u; k < g.n_partitions; k++) { duals[k] = cplex.getDual(colour[k]); } }

        return MpSolution{cplex.getObjValue(), columns, duals};
    }

    void MpSolver::add_mipstart(const std::vector<uint32_t>& mipstart_columns) const {
        if(lp) {
            throw std::runtime_error("Can't add a mipstart to an LP!");
        }

        IloNumVarArray initial_vars(env);
        IloNumArray initial_vals(env);

        for(auto id : mipstart_columns) {
            initial_vars.add(x[id]);
            initial_vals.add(1);
        }

        try {
            cplex.addMIPStart(initial_vars, initial_vals);
        } catch(IloException& e) {
            std::cout << e << std::endl;
            throw;
        }

        initial_vars.end();
        initial_vals.end();
    }

    boost::optional<MpSolution> MpSolver::solve(const std::vector<uint32_t>& forbidden_columns, bool skip_forbidden_columns_check, bool first_node_tilim) const {
        if(!skip_forbidden_columns_check) {
            for(auto cid = 0u; cid < c.size(); cid++) {
                if(std::find(forbidden_columns.begin(), forbidden_columns.end(), cid) == forbidden_columns.end()) {
                    if(lp) { x[cid].setUB(IloInfinity); } else { x[cid].setUB(1); }
                } else { x[cid].setUB(0); }
            }
        }

        auto old_tilim = cplex.getParam(IloCplex::TiLim);
        if(first_node_tilim) { cplex.setParam(IloCplex::TiLim, g.params.mip_heur_time_limit_first); }

        bool solved = false;

        try {
            solved = cplex.solve();
        } catch(const IloException& e) {
            cplex.setParam(IloCplex::TiLim, old_tilim);

            std::cerr << "CPLEX Raised an exception:" << std::endl;
            std::cerr << e << std::endl;
            throw;
        }

        cplex.setParam(IloCplex::TiLim, old_tilim);

        if(solved) {
            DEBUG_ONLY(std::cout << (lp ? "LP" : "MIP") << " Master Problem Solution: " << cplex.getObjValue() << std::endl;)
            auto ss = make_solution();
            return ss;
        } else {
            DEBUG_ONLY(std::cerr << (lp ? "LP" : "MIP") << " Master Problem Cplex error!" << std::endl;)
            DEBUG_ONLY(std::cerr << "\tStatus: " << cplex.getStatus() << std::endl;)
            DEBUG_ONLY(std::cerr << "\tSolver status: " << cplex.getCplexStatus() << std::endl;)

            if(lp) {
                cplex.exportModel("model.lp");
                throw std::runtime_error("LP problem infeasible: it should not be possible with the dummy column!");
            }

            return boost::none;
        }
    }

    void MpSolver::add_column(const StableSet& col) {
        std::stringstream name("");

        // Give a high price to the dummy column. For example,
        // 2 * number of vertices is more costly than any other
        // feasible solution (which uses at most 1 colour for
        // each partition).
        auto col_price = col.dummy ? (2 * g.n_vertices) : 1;

        IloNumColumn cpxcol = obj(col_price);

        for(auto k = 0u; k < g.n_partitions; ++k) {
            if(col.intersects(k)) { cpxcol += colour[k](1); }
        }

        name << "x_" << x.getSize();
        x.add(IloNumVar(cpxcol, 0, (lp ? IloInfinity : 1), (lp ? IloNumVar::Float : IloNumVar::Bool), name.str().c_str()));
    }

    boost::optional<MpSolution> MpSolver::solve_with_forbidden_check(const std::vector<uint32_t>& forbidden_columns) const {
        return solve(forbidden_columns, false, false);
    }

    boost::optional<MpSolution> MpSolver::solve_without_forbidden_check(const std::vector<uint32_t>& forbidden_columns) const {
        return solve(forbidden_columns, true, false);
    }

    boost::optional<MpSolution> MpSolver::solve_with_first_node_tilim(const std::vector<uint32_t>& forbidden_columns) const {
        return solve(forbidden_columns, true, true);
    }
}
