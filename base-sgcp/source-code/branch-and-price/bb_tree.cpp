#include "bb_tree.hpp"
#include "branching_rules.hpp"
#include "initial_solutions_generator.hpp"
#include "branching_helper.hpp"
#include "hoshino_populator.hpp"
#include "../utils/console_colour.hpp"
#include "../utils/dbg_output.hpp"
#include "../utils/cache.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <memory>
#include <chrono>
#include <cmath>

namespace sgcp {
    BBTree::BBTree(std::shared_ptr<const Graph> original_g) :
        original_g{original_g},
        bb_order{original_g->params},
        q{bb_order},
        lp_solver{*original_g, column_pool, true},
        mip_solver{*original_g, column_pool, false},
        stats{original_g->params}
    {
        auto initial_ub = generate_initial_pool();

        // This will be the id of the last BBNode solved
        last_node_id = 0u;

        // Initialise statistics
        stats.read_instance_data(*original_g);

        // Initialise the root node:

        // 1) The root node is the product of no branching rule
        std::shared_ptr<BranchingRule> no_branching = std::make_shared<EmptyRule>(original_g);

        // 2) No columns are forbidden at the root node
        auto no_forbidden = std::vector<uint32_t>{};

        // 3) Create the node
        auto root_node = BBNode{original_g, no_branching, column_pool, no_forbidden, initial_solution_ids, last_node_id++, 0u, boost::none, stats};

        // 4) Add it to the node queue
        q.insert(root_node);

        // Before solving the root node, we cannot say much on the LB...
        lb = 1;

        // The only UB we have available is from the best initial solution
        ub = static_cast<float>(initial_ub);
    }

    boost::optional<StableSetCollection> BBTree::solve() {
        using namespace Console;

        std::cout << std::endl << colour_magenta("Starting branch-and-price algorithm!") << std::endl << std::endl;

        std::cout << std::endl << "Node ID   LB        UB        Pool size     Open nodes" << std::endl;
        std::cout <<              "*---------*---------*---------*-------------*---------" << std::endl;

        while(!q.empty()) {
            if(stats.tot_computing_time > original_g->params.time_limit) {
                std::cout << Colour::Red << "Time limit hit! Aborting after " << stats.tot_computing_time << " seconds." << std::endl;
                break;
            }

            BBNode current_node = *q.begin();
            auto father_lb = current_node.bound_from_father;

            // Remove node from node list.
            q.erase(q.begin());

            DEBUG_ONLY(std::cout << Colour::Magenta << "Nodes in tree: " << q.size() + 1 << std::endl;)
            DEBUG_ONLY(std::cout << "Columns in global pool: " << column_pool.size() << Colour::Default << std::endl << std::endl;)

            if(father_lb && std::ceil(*father_lb) >= ub) {
                DEBUG_ONLY(std::cout << colour_red("Current node sub-optimal (deduced from father's LB): pruning.") << std::endl << std::endl;)
                continue;
            }

            auto sol = current_node.solve(ub, lp_solver, mip_solver);

            stats.max_depth_explored = std::max(current_node.depth, stats.max_depth_explored);

            if(!sol) {
                // Node infeasible
                DEBUG_ONLY(std::cout << colour_red("Current node infeasible: pruning.") << std::endl << std::endl;)
                continue;
            }

            if(!sol->timeout) {
                stats.nodes_solved++;

                if(sol->lb > ub + eps) {
                    // Node sub-optimal
                    DEBUG_ONLY(std::cout << colour_red("Current node sub-optimal: pruning.") << std::endl << std::endl;)
                    continue;
                }

                if(sol->node_solved_to_optimality()) {
                    // Node solved to optimality => will not have children
                    DEBUG_ONLY(std::cout << colour_green("Node solved to optimality.") << std::endl << std::endl;)

                    if(sol->integer_solution_columns.empty()) {
                        DEBUG_ONLY(
                            std::cout << colour_magenta("Solution at node: ") << std::endl;
                            for(auto cidval : sol->integer_solution_columns) {
                                assert(cidval.second > 1 - eps);
                                const StableSet& s = column_pool.at(cidval.first);
                                std::cout << "\t" << s << std::endl;
                            }
                            std::cout << std::endl;
                        )
                    }
                } else {
                    DEBUG_ONLY(std::cout << Colour::Magenta << "Solution fractional at the node: lb = " << sol->lb << ", ub = " << sol->ub << "." << Colour::Default << std::endl << std::endl;)
                    branch(current_node, *sol);
                }

                // Update bounds
                update_bounds(*sol);

                if(stats.nodes_solved == 1 || stats.nodes_solved % original_g->params.print_bb_stats_every_n_nodes == 0) {
                    std::cout << std::left;
                    std::cout << std::setw(10) << stats.nodes_solved;
                    std::cout << std::setw(10) << lb;
                    std::cout << std::setw(10) << ub;
                    std::cout << std::setw(14) << column_pool.size();
                    std::cout << q.size() << std::endl;
                }
            } else {
                // If the node timed-out, it can have given us a new UB, if it solved the MIP
                if(sol->ub < ub) {
                    ub = sol->ub;
                    best_solution.clear();
                    for(auto cidval : sol->integer_solution_columns) { best_solution.push_back(column_pool.at(cidval.first)); }
                }

                // However, it can also give us info on the LB, if we calculated the Lagrangean bound
                if(sol->lb > lb) { lb = sol->lb; }

                // Here we just let the loop continue, as at the next iteration it will realise
                // that a timeout occurred and will exit graciously. We don't delete the current
                // node, so to leave it open in the tree.
                continue;
            }
        }

        stats.nodes_open = q.size();
        stats.column_pool_size = column_pool.size();
        stats.ub = ub;
        stats.lb = lb;
        stats.build_stats();

        std::cout << std::endl << yellow_separator() << std::endl;
        std::cout << Colour::Yellow << "BB Tree exploration completed!" << std::endl;
        std::cout << "Lower bound: " << lb << " (=> " << std::ceil(lb) << ")" << std::endl;
        std::cout << "Upper bound: " << ub << Colour::Default << std::endl;

        if(best_solution.empty()) { return boost::none; }
        return best_solution;
    }

    void BBTree::update_bounds(const BBSolution& sol) {
        if(!q.empty() && q.begin()->bound_from_father) { lb = std::max(lb, *(q.begin()->bound_from_father)); }
        else { lb = std::max(lb, sol.lb); }

        if(sol.ub < ub) {
            ub = sol.ub;
            best_solution.clear();
            for(auto cidval : sol.integer_solution_columns) { best_solution.push_back(column_pool.at(cidval.first)); }
        }
    }

    void BBTree::branch(const BBNode& n, const BBSolution& sol) {
        bool branched = false;

        // Only try branch_on_vertex_in_partition if we are solving a
        // "proper" SGCP; don't even try, if our problem is a GCP.
        if(original_g->n_vertices > original_g->n_partitions) {
            branched = branch_on_vertex_in_partition(n, sol);
        }

        if(!branched) { branched = branch_on_edge(n, sol); }

        assert(branched);
    }

    bool BBTree::branch_on_vertex_in_partition(const BBNode& n, const BBSolution& sol) {
        using namespace Console;

        std::shared_ptr<const Graph> g = sol.g;
        BranchingHelper bh{*g, sol, column_pool};
        assert(g->n_partitions == original_g->n_partitions);

        auto part_and_vertex = bh.most_fractional_vertex_in_partition_with_more_than_one_coloured_vertex();

        // If there is no partition with more than one coloured vertex, this branching rule
        // cannot be used.
        if(!part_and_vertex) { return false; }

        DEBUG_ONLY(std::cout << Colour::Yellow << "Branching on which vertex to colour in partition " << part_and_vertex->first << std::endl;)
        auto chosen_v = g->vertex_by_id(part_and_vertex->second);
        assert(chosen_v);
        DEBUG_ONLY(std::cout << "Vertex: " << g->g[*chosen_v] << std::endl;)
        DEBUG_ONLY(std::cout << yellow_separator() << Colour::Default << std::endl << std::endl;)

        // Create the first branch: colour chosen_id
        std::vector<uint32_t> chosen_vertex_id = {part_and_vertex->second};
        std::shared_ptr<BranchingRule> vr1 = std::make_shared<VerticesRemoveRule>(sol.g, chosen_vertex_id);
        BBNode new_node_1{original_g, vr1, column_pool, n.forbidden_columns, initial_solution_ids, last_node_id++, n.depth + 1, sol.lb, stats};

        // Create the second branch: colour a node != chosen_id
        std::vector<uint32_t> other_vertices_id;
        for(const auto& v_id : g->p[part_and_vertex->first]) {
            if(v_id != part_and_vertex->second) { other_vertices_id.push_back(v_id); }
        }
        std::shared_ptr<BranchingRule> vr2 = std::make_shared<VerticesRemoveRule>(sol.g, other_vertices_id);
        BBNode new_node_2{original_g, vr2, column_pool, n.forbidden_columns, initial_solution_ids, last_node_id++, n.depth + 1, sol.lb, stats};

        q.insert(new_node_1);
        q.insert(new_node_2);

        stats.n_branch_on_coloured_v++;

        return true;
    }

    bool BBTree::branch_on_edge(const BBNode& n, const BBSolution& sol) {
        using namespace Console;

        std::shared_ptr<const Graph> g = sol.g;
        BranchingHelper bh{*g, sol, column_pool};
        assert(g->n_partitions == original_g->n_partitions);

        // First, find the most fractional column
        auto column1_id = bh.most_fractional_column();

        // Second, find any vertex covered by the column
        auto id_i = bh.any_vertex_in_set(column_pool.at(column1_id).get_set());
        assert(id_i);

        auto column2_id = bh.another_column_covering_vertex(column1_id, *id_i);
        assert(column2_id);

        // Fourth, find a vertex covered by only one of the two columns
        auto id_j = bh.any_vertex_covered_by_exactly_one_column(column1_id, *column2_id);
        assert(id_j);
        assert(*id_j != *id_i);
        assert(!g->connected(*id_i, *id_j));

        auto v_i = g->vertex_by_id(*id_i);
        assert(v_i);
        auto v_j = g->vertex_by_id(*id_j);
        assert(v_j);

        DEBUG_ONLY(std::cout << Colour::Yellow << "Branching on vertices covered by two columns" << std::endl;)
        DEBUG_ONLY(std::cout << "Vertex 1: " << g->g[*v_i] << std::endl;)
        DEBUG_ONLY(std::cout << "Vertex 2: " << g->g[*v_j] << std::endl;)
        DEBUG_ONLY(std::cout << yellow_separator() << Colour::Default << std::endl << std::endl;)

        // Create the first branch: merge i and j
        std::shared_ptr<BranchingRule> vm = std::make_shared<VerticesMergeRule>(g, *id_i, *id_j);
        BBNode new_node_1{original_g, vm, column_pool, n.forbidden_columns, initial_solution_ids, last_node_id++, n.depth + 1, sol.lb, stats};
        q.insert(new_node_1);

        // Create the second branch: link i and j
        std::shared_ptr<BranchingRule> vl = std::make_shared<VerticesLinkRule>(g, *id_i, *id_j);
        BBNode new_node_2{original_g, vl, column_pool, n.forbidden_columns, initial_solution_ids, last_node_id++, n.depth + 1, sol.lb, stats};
        q.insert(new_node_2);

        stats.n_branch_on_edge++;

        return true;
    }

    uint32_t BBTree::generate_initial_pool() {
        using namespace std::chrono;

        auto init_start_time = high_resolution_clock::now();
        stats.heuristic_ub = original_g->n_partitions;

        if(original_g->params.use_initial_solution) {
            auto init_sol_gen = InitialSolutionsGenerator{*original_g, stats};
            auto init_sol = init_sol_gen.generate();
            stats.heuristic_ub = init_sol.feasible_solution_ids.size();

            // Initialise the column pool with columns from the initial heuristics
            column_pool = init_sol.columns;

            // Keep track of which columns form the best feasible solution encountered
            initial_solution_ids = init_sol.feasible_solution_ids;

            // Save the best initial solution as the best overall solution so far
            for (auto id : initial_solution_ids) { best_solution.push_back(column_pool[id]); }

            // Add more columns applying Hoshino's ``populate'' method
            if(original_g->params.use_populate) {
                HoshinoPopulator pop{*original_g, column_pool};
                auto pop_columns = pop.enlarge_pool();
                for (const auto &c : pop_columns) { column_pool.push_back(c); }
            }

            // Check the bks
            cache::bks_update_pool(column_pool, *original_g);
        }

        // Add the dummy column
        auto dummy_col = StableSet{*original_g};
        column_pool.push_back(dummy_col);

        // Add the columns to the LP and MIP
        for(const auto& c : column_pool) { lp_solver.add_column(c); mip_solver.add_column(c); }

        // Use the best initial solution as a MIPstart
        if(original_g->params.use_initial_solution) {
            mip_solver.add_mipstart(initial_solution_ids);
        }

        auto init_end_time = high_resolution_clock::now();
        stats.tot_computing_time = duration_cast<duration<float>>(init_end_time - init_start_time).count();

        return stats.heuristic_ub;
    }

    void BBTree::write_results() const {
        stats.write_csv();
    }
}
