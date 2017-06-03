#include "params.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace sgcp {
    Params::Params(std::string filename) {
        using namespace boost::property_tree;

        ptree tree;
        read_json(filename, tree);

        time_limit = tree.get<uint32_t>("branch_and_price.time_limit");
        cplex_threads = tree.get<uint32_t>("branch_and_price.cplex_threads");
        mp_time_limit = tree.get<uint32_t>("branch_and_price.mp_time_limit");

        use_initial_solution = tree.get<bool>("branch_and_price.use_initial_solution");
        use_populate = tree.get<bool>("branch_and_price.use_populate");

        mip_heur_active = tree.get<bool>("branch_and_price.mip_heuristic.active");
        mip_heur_alns = tree.get<bool>("branch_and_price.mip_heuristic.alns");
        mip_heur_time_limit = tree.get<uint32_t>("branch_and_price.mip_heuristic.time_limit");
        mip_heur_time_limit_first = tree.get<uint32_t>("branch_and_price.mip_heuristic.time_limit_first");
        mip_heur_max_cols = tree.get<uint32_t>("branch_and_price.mip_heuristic.max_cols");
        mip_heur_frequency = tree.get<uint32_t>("branch_and_price.mip_heuristic.frequency");

        mwss_multiplier = tree.get<uint32_t>("mwss_multiplier");

        tabu_iterations = tree.get<uint32_t>("tabu.iterations");
        tabu_instance_scaled_iters = tree.get<bool>("tabu.instance_scaled_iters");
        tabu_tenure = tree.get<uint32_t>("tabu.tenure");
        std::string tabu_score = tree.get<std::string>("tabu.score");

        alns_iterations = tree.get<uint32_t>("alns.iterations");
        alns_instance_scaled_iters = tree.get<bool>("alns.instance_scaled_iters");
        alns_new_best_mult = tree.get<float>("alns.new_best_mult");
        alns_new_improving_mult = tree.get<float>("alns.new_improving_mult");
        alns_worsening_mult = tree.get<float>("alns.worsening_mult");
        alns_wa_initial_probability = tree.get<float>("alns.wa_initial_probability");
        alns_acceptance = tree.get<std::string>("alns.acceptance");
        alns_local_search = tree.get<std::string>("alns.local_search");

        for(const ptree::value_type& el : tree.get_child("alns.dmoves")) {
            uint32_t move = el.second.get<uint32_t>("");
            assert(move == 0u || move == 1u);
            alns_dmoves.push_back(move);
        }

        for(const ptree::value_type& el : tree.get_child("alns.rmoves")) {
            uint32_t move = el.second.get<uint32_t>("");
            assert(move == 0u || move == 1u);
            alns_rmoves.push_back(move);
        }

        grasp_iterations = tree.get<uint32_t>("grasp.iterations");
        grasp_threads = tree.get<uint32_t>("grasp.threads");

        results_dir = tree.get<std::string>("results.results_dir");
        results_file = tree.get<std::string>("results.results_file");
        print_bb_stats_every_n_nodes = tree.get<uint32_t>("results.print_bb_stats_every_n_nodes");

        decomposition_first_stage_time_limit = tree.get<uint32_t>("decomposition.first_stage_time_limit");
        decomposition_lifting_coeff = tree.get<uint32_t>("decomposition.lifting_coeff");
        decomposition_max_added_cuts_when_caching = tree.get<uint32_t>("decomposition.max_added_cuts_when_caching");
        decomposition_3cuts_strategy = tree.get<std::string>("decomposition.3cuts_strategy");

        std::string bb_explo = tree.get<std::string>("branch_and_price.bb_exploration_strategy");
        if(bb_explo == "depth-first") {
            bb_exploration_strategy = BBExplorationStrategy::DepthFirst;
        } else {
            assert(bb_explo == "best-first");
            bb_exploration_strategy = BBExplorationStrategy::BestFirst;
        }
    }
}