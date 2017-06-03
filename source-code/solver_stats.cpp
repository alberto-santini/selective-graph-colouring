#include "solver_stats.hpp"

#include <fstream>
#include <numeric>
#include <cmath>

// If boost::filesystem worked decently on OS X:
#include <boost/filesystem.hpp>
// Workaround:
#include <libgen.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace sgcp {
    void SolverStats::reset() {
        n_vertices = 0u;
        n_edges = 0u;
        n_partitions = 0u;
        nodes_solved = 0u;
        nodes_open = 0u;
        max_depth_explored = 0u;
        n_branch_on_coloured_v = 0u;
        n_branch_on_edge = 0u;
        col_generated_by_pricing_at_root = 0u;
        column_pool_size = 0u;

        density = 0.0f;
        avg_partition_size = 0.0f;
        ub = 0.0f;
        lb = 0.0f;
        gap = 0.0f;
        heuristic_ub = 0.0f;
        ub_after_root_pricing = 0.0f;
        lb_after_root_pricing = 0.0f;
        ub_after_root_overall = 0.0f;
        gap_after_root_pricing = 0.0f;
        gap_after_root_overall = 0.0f;
        tot_computing_time = 0.0f;
        root_node_time = 0.0f;
        tot_lp_time = 0.0f;
        tot_pricing_time = 0.0f;
        avg_col_generated_by_pricing_excl_root = 0.0f;

        num_pri_cols = std::vector<uint32_t>{};

        instance = "";
    }

    void SolverStats::read_instance_data(const Graph& g) {
        instance = g.data_filename;
        n_vertices = g.n_vertices;
        n_edges = g.n_edges;
        n_partitions = g.n_partitions;
    }

    void SolverStats::build_stats() {
        auto sum = [] (const auto& begin, const auto& end) { return std::accumulate(begin, end, 0.0f); };
        auto avg = [&sum] (const auto& begin, const auto& end) { return static_cast<float>(sum(begin, end)) / std::distance(begin, end); };

        density = 2.0f * static_cast<float>(n_edges) / (n_vertices * (n_vertices - 1));
        avg_partition_size = static_cast<float>(n_vertices) / n_partitions;
        lb = std::ceil(lb);
        ub = std::floor(ub);
        gap = (ub - lb) / ub;
        gap_after_root_pricing = (ub_after_root_pricing - lb_after_root_pricing) / ub_after_root_pricing;
        gap_after_root_overall = (ub_after_root_overall - lb_after_root_pricing) / ub_after_root_overall;
        tot_computing_time = std::min(tot_computing_time, static_cast<float>(params.time_limit));

        col_generated_by_pricing_at_root = num_pri_cols.at(0);

        if(num_pri_cols.size() > 1u) {
            avg_col_generated_by_pricing_excl_root = avg(std::next(num_pri_cols.begin()), num_pri_cols.end());
        } else {
            avg_col_generated_by_pricing_excl_root = 0.0f;
        }

        // If boost::filesystem worked decently on OS X:
        // instance = basename(boost::filesystem::path(instance)).string()
        // Workaround:
        std::vector<char> fn(instance.begin(), instance.end());
        fn.push_back('\0');
        instance = basename(&fn[0]);
        instance = instance.substr(0, instance.find_last_of("."));
    }

    void SolverStats::write_csv() const {
        // If boost::filesystem worked decently on OS X:
        // boost::filesystem::path dir(params.results_dir);
        // boost::filesystem::path file(params.results_file);
        // auto filename = (dir / file).string();
        // Workaround:
        auto filename = params.results_dir + "/" + params.results_file;

        std::ofstream f(filename, std::ios::out | std::ios::app);

        f << instance << ",";
        f << n_vertices << ",";
        f << n_edges << ",";
        f << n_partitions << ",";
        f << nodes_solved << ",";
        f << nodes_open << ",";
        f << max_depth_explored << ",";
        f << n_branch_on_coloured_v << ",";
        f << n_branch_on_edge << ",";
        f << col_generated_by_pricing_at_root << ",";
        f << avg_col_generated_by_pricing_excl_root << ",";
        f << column_pool_size << ",";
        f << heuristic_ub << ",";
        f << ub_after_root_pricing << ",";
        f << ub_after_root_overall << ",";
        f << ub << ",";
        f << lb_after_root_pricing << ",";
        f << lb << ",";
        f << gap_after_root_pricing << ",";
        f << gap_after_root_overall << ",";
        f << gap << ",";
        f << tot_computing_time << ",";
        f << root_node_time << std::endl;

        f.close();
    }
}
