#ifndef _BB_TREE_HPP
#define _BB_TREE_HPP

#include "../graph.hpp"
#include "../solver_stats.hpp"
#include "column_pool.hpp"
#include "bb_node.hpp"

#include <queue>
#include <memory>

#include <vector>
#include <set>
#include <map>

namespace sgcp {
    struct BestFirstOrder {
        bool operator()(const BBNode& lhs, const BBNode& rhs) {
            if(!lhs.bound_from_father) { return true; }
            if(!rhs.bound_from_father) { return false; }
            return std::tie(*lhs.bound_from_father, lhs.node_id) < std::tie(*rhs.bound_from_father, rhs.node_id);
        }
    };

    struct DepthFirstOrder {
        bool operator()(const BBNode& lhs, const BBNode& rhs) {
            if(lhs.depth != rhs.depth) {
                return lhs.depth > rhs.depth;
            }

            return BestFirstOrder()(lhs, rhs);
        }
    };

    struct BBOrder {
        const Params& p;
        BestFirstOrder bf;
        DepthFirstOrder df;

        BBOrder(const Params& p) : p{p} {}

        bool operator()(const BBNode& lhs, const BBNode& rhs) {
            if(p.bb_exploration_strategy == BBExplorationStrategy::DepthFirst) { return df(lhs, rhs); }
            return bf(lhs,rhs);
        }
    };

    class BBTree {
        std::shared_ptr<const Graph> original_g;
        ColumnPool column_pool;

        BBOrder bb_order;
        std::set<BBNode, BBOrder> q;
        std::vector<uint32_t> initial_solution_ids;

        MpSolver lp_solver;
        MpSolver mip_solver;

        float lb;
        float ub;
        StableSetCollection best_solution;

        // Node id of the last created node.
        uint32_t last_node_id;

        SolverStats stats;

        static constexpr float eps = 1e-6;

        uint32_t generate_initial_pool();
        void update_bounds(const BBSolution& sol);
        void branch(const BBNode& n, const BBSolution& sol);
        bool branch_on_vertex_in_partition(const BBNode& n, const BBSolution& sol);
        bool branch_on_edge(const BBNode& n, const BBSolution& sol);

    public:
        BBTree(std::shared_ptr<const Graph> original_g);

        boost::optional<StableSetCollection> solve();
        void write_results() const;
    };
}

#endif