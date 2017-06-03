#include "bb_node.hpp"
#include "../mwss/mwss_solver.hpp"
#include "../utils/console_colour.hpp"
#include "../utils/dbg_output.hpp"
#include "initial_solutions_generator.hpp"

#include <cmath>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace sgcp {
    BBNode::BBNode( std::shared_ptr<const Graph> o,
                    std::shared_ptr<const BranchingRule> br,
                    ColumnPool& c,
                    std::vector<uint32_t> forbidden_columns,
                    std::vector<uint32_t> initial_solution_ids,
                    uint32_t node_id,
                    uint32_t depth,
                    boost::optional<float> bound_from_father,
                    SolverStats& stats) :
                    o{o},
                    br{br},
                    g{nullptr},
                    c{c},
                    forbidden_columns{forbidden_columns},
                    initial_solution_ids{initial_solution_ids},
                    node_id{node_id},
                    depth{depth},
                    bound_from_father{bound_from_father},
                    stats{stats}
    {
        for(auto cid = 0u; cid < this->c.get().size(); cid++) {
            if(std::find(forbidden_columns.begin(), forbidden_columns.end(), cid) != forbidden_columns.end()) { continue; }
            if(!br->is_compatible(this->c.get().at(cid))) { this->forbidden_columns.push_back(cid); }
            else { this->whitelisted_columns.push_back(cid); }
        }
    }

    void BBNode::check_new_columns() {
        for(auto cid = 0u; cid < c.get().size(); cid++) {
            if(std::find(forbidden_columns.begin(), forbidden_columns.end(), cid) != forbidden_columns.end()) { continue; }
            if(std::find(whitelisted_columns.begin(), whitelisted_columns.end(), cid) != whitelisted_columns.end()) { continue; }
            if(!g->is_compatible_as_stable_set(c.get().at(cid).get_set())) { forbidden_columns.push_back(cid); }
        }
    }

    boost::optional<BBSolution> BBNode::solve(float ub, MpSolver& lp_solver, MpSolver& mip_solver) {
        DEBUG_ONLY(using namespace Console;)
        using namespace std::chrono;

        // Build the local graph
        assert(g == nullptr);
        g = br->apply();

        std::vector<StableSet> new_columns;

        DEBUG_ONLY(std::cout << Colour::Magenta << "Node id: " << node_id << ", depth: " << depth << Colour::Default << std::endl << std::endl;)

        check_new_columns();

        stats.get().num_pri_cols.push_back(0u);

        bool first_run = true;
        float last_pricing_violation = 0;

        while(true) {
            auto lp_start_time = high_resolution_clock::now();

            boost::optional<MpSolution> mp_solution;
            if(first_run) { mp_solution = lp_solver.solve_with_forbidden_check(forbidden_columns); }
            else { mp_solution = lp_solver.solve_without_forbidden_check(forbidden_columns); }

            auto lp_end_time = high_resolution_clock::now();
            auto lp_time_s = duration_cast<duration<float>>(lp_end_time - lp_start_time).count();

            // Solving the LP should always produce a solution (worst case there is the dummy column).
            assert(mp_solution);

            stats.get().tot_computing_time += lp_time_s;
            stats.get().tot_lp_time += lp_time_s;

            if(node_id == 0u) {
                stats.get().root_node_time = stats.get().tot_computing_time;
            }

            // If the exploration was interrupted by a timeout, report so.
            if(stats.get().tot_computing_time > g->params.time_limit) {
                auto sol = get_bbsolution(lp_solver, *mp_solution, new_columns, ub);

                // Compute the Lagrange bound:
                if(last_pricing_violation > 1 - eps) { sol.lb = std::ceil(mp_solution->obj_value / last_pricing_violation); }

                sol.timeout = true;
                return sol;
            }

            first_run = false;

            auto mwss_solver = MwssSolver{*o, *g};

            auto pricing_start_time = high_resolution_clock::now();
            auto sp_columns = mwss_solver.solve(*mp_solution);
            auto pricing_end_time = high_resolution_clock::now();
            auto pricing_time_s = duration_cast<duration<float>>(pricing_end_time - pricing_start_time).count();

            stats.get().tot_computing_time += pricing_time_s;
            stats.get().tot_pricing_time += pricing_time_s;

            bool new_valid_columns = false;

            for(const auto& col : sp_columns) {
                last_pricing_violation = col.reduced_cost(mp_solution->duals);

                if(last_pricing_violation > min_reduced_cost + eps) {
                    DEBUG_ONLY(std::cout << "\tNew column generated: " << Colour::Green << col << Colour::Default << " (reduced cost: " << colour_magenta(col.reduced_cost(mp_solution->duals)) << ")" << std::endl;)

                    assert(std::none_of(c.get().begin(), c.get().end(), [&] (const auto& lcol) { return lcol == col; }));

                    new_valid_columns = true;
                    new_columns.push_back(col);
                    c.get().push_back(col);
                    lp_solver.add_column(col);
                    mip_solver.add_column(col);
                    stats.get().num_pri_cols.back()++;
                } else {
                    DEBUG_ONLY(std::cout << "\tNew column discarded: " << Colour::Red << col << Colour::Default << " (reduced cost: " << colour_magenta(col.reduced_cost(mp_solution->duals)) << ")" << std::endl;)
                }
                DEBUG_ONLY(std::cout << std::endl;)
            }

            if(!new_valid_columns) {
                // If the solution has the dummy column in its base columns, consider it as infeasible.
                for(const auto& col_val : mp_solution->columns) {
                    if(col_val.first.dummy && col_val.second > eps) { return boost::none; }
                }

                return get_bbsolution(mip_solver, *mp_solution, new_columns, ub);
            }
        }

        return boost::none;
    }

    BBSolution BBNode::get_bbsolution(const MpSolver& mip_solver, const MpSolution& mp_solution, const std::vector<StableSet>& new_columns, float ub) const {
        DEBUG_ONLY(using namespace Console;)
        using namespace std::chrono;

        float lb = mp_solution.obj_value;
        ActiveColumnsWithCoeff integer_solution_columns{};
        ActiveColumnsWithCoeff lp_solution_columns = mp_solution.active_columns_by_id(c);

        if(mp_solution.is_integer()) {
            ub = std::min(ub, mp_solution.obj_value);
            integer_solution_columns = lp_solution_columns;

            if(node_id == 0u) {
                stats.get().ub_after_root_pricing = ub;
                stats.get().ub_after_root_overall = ub;
                stats.get().lb_after_root_pricing = lb;
            }

            return BBSolution{ub, lb, integer_solution_columns, lp_solution_columns, forbidden_columns, g};
        }

        if(node_id == 0u) {
            stats.get().ub_after_root_pricing = ub;
            stats.get().lb_after_root_pricing = lb;
        }

        // We want to try to obtain an UB by solving a MIP
        // with a black-box solver, using the columns
        // generated so far.
        auto mip_act = g->params.mip_heur_active;

        // If the LP obj value is, e.g. 3.5 and the UB is 4,
        // do not solve the MIP, because you can't get anything
        // better than the current UB.
        auto can_improve = std::ceil(mp_solution.obj_value) < ub - 1 - eps;

        // If no new column was added, this MIP has might have been
        // solved somewhere else, do not solve it again!
        auto new_cols = !new_columns.empty();

        // Solve the MIP when having fewer columns than this
        auto num_cols_ok = c.get().size() <= g->params.mip_heur_max_cols;

        // Solve the MIP each N nodes (including the root node).
        auto node_id_ok = node_id % g->params.mip_heur_frequency == 0u;

        if(mip_act && (node_id_ok || (can_improve && new_cols && num_cols_ok))) {
            auto mip_start_time = high_resolution_clock::now();

            boost::optional<MpSolution> mip_sol;

            if(node_id == 0u) { mip_sol = mip_solver.solve_with_first_node_tilim({}); }
            else { mip_sol = mip_solver.solve_without_forbidden_check({}); }

            auto mip_end_time = high_resolution_clock::now();
            auto mip_time_s = duration_cast<duration<float>>(mip_end_time - mip_start_time).count();

            stats.get().tot_computing_time += mip_time_s;

            auto mip_cols = mip_sol->active_columns_by_id(c);

            // The MIP solution is feasible iff it does not contain the dummy column.
            auto mip_sol_feasible = mip_sol &&
                std::all_of(
                mip_cols.begin(),
                mip_cols.end(),
                [this] (const auto& id_coeff) { return (id_coeff.second < 0.5 || !c.get().at(id_coeff.first).dummy); }
            );

            if(mip_sol_feasible) {
                ub = std::min(ub, mip_sol->obj_value);
                integer_solution_columns = mip_cols;

                if( g->params.mip_heur_alns &&
                    ub - std::ceil(lb) > .5 // Otherwise we just found the optimal solution with MIP, no need for ALNS
                ) {
                    // Try to improve on the MIP solution with ALNS
                    ColumnPool initial_solution;
                    for(const auto& cid_val : integer_solution_columns) {
                        initial_solution.push_back(c.get().at(cid_val.first));
                    }

                    auto init_sol_gen = InitialSolutionsGenerator{*o, stats};
                    auto sol = init_sol_gen.generate_from_existing(initial_solution);

                    if(sol.columns.size() < ub) {
                        integer_solution_columns = ActiveColumnsWithCoeff{};

                        for(auto id : sol.feasible_solution_ids) {
                            boost::optional<uint32_t> id_in_column_pool = boost::none;

                            const auto& s = sol.columns.at(id);

                            for(auto cid = 0u; cid < c.get().size(); ++cid) {
                                if(c.get().at(cid) == s) {
                                    id_in_column_pool = cid;
                                    break;
                                }
                            }

                            if(id_in_column_pool) {
                                integer_solution_columns[*id_in_column_pool] = 1.0;
                            } else {
                                c.get().push_back(s);
                                integer_solution_columns[c.get().size() - 1] = 1.0;
                            }
                        }

                        assert(integer_solution_columns.size() == sol.columns.size());

                        ub = sol.columns.size();
                    }
                }
            }
        }

        if(node_id == 0u) {
            stats.get().ub_after_root_overall = ub;
        }

        return BBSolution{ub, lb, integer_solution_columns, lp_solution_columns, forbidden_columns, g};
    }
}
