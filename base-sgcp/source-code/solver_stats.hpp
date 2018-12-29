#ifndef _SOLVER_STATS_HPP
#define _SOLVER_STATS_HPP

#include "params.hpp"
#include "graph.hpp"

#include <string>
#include <vector>

namespace sgcp {
    struct SolverStats {
        const Params& params;

        uint32_t n_vertices;
        uint32_t n_edges;
        uint32_t n_partitions;
        uint32_t nodes_solved;
        uint32_t nodes_open;
        uint32_t max_depth_explored;
        uint32_t n_branch_on_coloured_v;
        uint32_t n_branch_on_edge;
        uint32_t col_generated_by_pricing_at_root;
        uint32_t column_pool_size;

        float density;
        float avg_partition_size;
        float ub;
        float lb;
        float gap;
        float heuristic_ub;
        float ub_after_root_pricing;
        float lb_after_root_pricing;
        float ub_after_root_overall;
        float gap_after_root_pricing;
        float gap_after_root_overall;
        float tot_computing_time;
        float root_node_time;
        float tot_lp_time;
        float tot_pricing_time;
        float avg_col_generated_by_pricing_excl_root;

        std::vector<uint32_t> num_pri_cols;

        std::string instance;

        SolverStats(const Params& params) : params{params} { reset(); }

        void reset();
        void read_instance_data(const Graph& g);
        void build_stats();
        void write_csv() const;
    };
}


#endif