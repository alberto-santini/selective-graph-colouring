#ifndef _PARAMS_HPP
#define _PARAMS_HPP

#include <string>
#include <vector>

namespace sgcp {
    enum class BBExplorationStrategy : uint32_t {
        BestFirst,
        DepthFirst
    };

    struct Params {
        uint32_t time_limit;
        uint32_t cplex_threads;
        uint32_t mp_time_limit;

        BBExplorationStrategy bb_exploration_strategy;

        bool use_initial_solution;
        bool use_populate;

        bool mip_heur_active;
        bool mip_heur_alns;
        uint32_t mip_heur_time_limit;
        uint32_t mip_heur_time_limit_first;
        uint32_t mip_heur_max_cols;
        uint32_t mip_heur_frequency;

        uint32_t mwss_multiplier;

        uint32_t tabu_iterations;
        uint32_t tabu_tenure;
        bool tabu_instance_scaled_iters;
        std::string tabu_score;

        uint32_t alns_iterations;
        bool alns_instance_scaled_iters;
        float alns_new_best_mult;
        float alns_new_improving_mult;
        float alns_worsening_mult;
        float alns_wa_initial_probability;
        std::string alns_acceptance;
        std::string alns_local_search;
        std::vector<uint32_t> alns_dmoves;
        std::vector<uint32_t> alns_rmoves;

        uint32_t grasp_iterations;
        uint32_t grasp_threads;

        std::string results_dir;
        std::string results_file;
        uint32_t print_bb_stats_every_n_nodes;

        uint32_t decomposition_first_stage_time_limit;
        uint32_t decomposition_lifting_coeff;
        uint32_t decomposition_max_added_cuts_when_caching;
        std::string decomposition_3cuts_strategy;

        Params(std::string filename);
    };
}

#endif
