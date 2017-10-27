#include "graph.hpp"
#include "stable_set.hpp"
#include "solver_stats.hpp"
#include "branch-and-price/bb_tree.hpp"
#include "campelo-mip/campelo_mip_solver.hpp"
#include "compact-mip/compact_mip_solver.hpp"
#include "decomposition/decomposition_solver.hpp"
#include "heuristics/alns/alns.hpp"
#include "heuristics/tabu_search.hpp"
#include "heuristics/greedy_heuristic.hpp"
#include "heuristics/grasp.hpp"

#include <chrono>
#include <utils/cache.hpp>

std::array<std::string, 10> solvers = {
    "bp", // Branch-and-price
    "campelo", // Campelo's representatives model
    "compact", // Compact formulation without representatives
    "greedy", // Greedy initial heuristics
    "alns", // ALNS heuristic
    "alns-stats", // ALNS heuristic with stats
    "tabu", // TABU Search heuristic
    "grasp", // GRASP heuristic
    "decomposition", // Benders-like decomposition solver
    "info", // Just print graph info
};

bool file_exists(const std::string& file_name) {
    std::ifstream fs(file_name.c_str());
    return fs.good();
}

bool valid_solver(const std::string& solver) {
    return std::find(solvers.begin(), solvers.end(), solver) != solvers.end();
}

void print_solution(boost::optional<sgcp::StableSetCollection> solution) {
    if(solution) {
        std::cout << std::endl << "=== Solution ===" << std::endl;
        std::cout << *solution << std::endl;
    } else {
        std::cout << "No solution!" << std::endl;
    }
}

void solve_bp(std::shared_ptr<sgcp::Graph> g) {
    sgcp::BBTree bb_tree{g};
    auto sol = bb_tree.solve();

    // Write statistics and results to file:
    bb_tree.write_results();

    print_solution(sol);
    sgcp::cache::bks_update_cache(*sol, *g);
}

void solve_campelo(std::shared_ptr<sgcp::Graph> g) {
    sgcp::CampeloMipSolver solver{*g};
    auto sol = solver.solve();
    print_solution(sol);
}

void solve_compact(std::shared_ptr<sgcp::Graph> g) {
    sgcp::CompactMipSolver solver{*g};
    auto sol = solver.solve();
    print_solution(sol);
}

void solve_greedy(std::shared_ptr<sgcp::Graph> g) {
    sgcp::GreedyHeuristicSolver solver{*g};
    auto sol = solver.solve();
    std::cout << g->data_filename << "," << sol.size() << std::endl;
}

void solve_alns(std::shared_ptr<sgcp::Graph> g, bool print_stats) {
    sgcp::ALNSSolver solver{*g};
    sgcp::ALNSStats stats;
    float elapsed_time = 0;
    auto sol = solver.solve(boost::none, &elapsed_time, print_stats ? &stats : nullptr);

    if(print_stats) {
        stats.print_stats();
    } else {
        std::cout << g->data_filename << ","
                  << g->params.alns_acceptance << ","
                  << g->params.tabu_tenure << ","
                  << g->params.alns_wa_initial_probability << ","
                  << elapsed_time << ","
                  << sol.n_colours << std::endl;
    }

    sgcp::cache::bks_update_cache(sol.to_column_pool(), *g);
}

void solve_tabu(std::shared_ptr<sgcp::Graph> g) {
    sgcp::TabuSearchSolver solver{*g};
    float elapsed_time = 0;
    auto sol = solver.solve(boost::none, &elapsed_time);

    std::cout << g->data_filename << ","
              << elapsed_time << ","
              << sol.size() << std::endl;

    sgcp::cache::bks_update_cache(sol, *g);
}

void solve_grasp(std::shared_ptr<sgcp::Graph> g) {
    using namespace std::chrono;

    sgcp::GRASPSolver solver{*g};

    auto stime = high_resolution_clock::now();
    auto sol = solver.solve();
    auto etime = high_resolution_clock::now();
    auto elapsed_time = duration_cast<duration<float>>(etime - stime).count();

    std::cout << g->data_filename << ","
              << elapsed_time << ","
              << sol.size() << std::endl;
}

void solve_decomposition(std::shared_ptr<sgcp::Graph> g) {
    auto c = sgcp::DecompositionSolver{*g};
    c.solve();
}

int main(int argc [[maybe_unused]], char* argv[]) {
    using namespace sgcp;

    std::string params_file = argv[1];
    std::string instance_file = argv[2];
    std::string solver = argv[3];

    if(!file_exists(params_file)) {
        std::cerr << "Cannot find params file: " << params_file << std::endl;
        return 1;
    }

    if(!file_exists(instance_file)) {
        std::cerr << "Cannot find instance file: " << instance_file << std::endl;
        return 1;
    }

    if(!valid_solver(solver)) {
        std::cerr << "Solver not valid: " << solver << std::endl;
        return 1;
    }

    auto g = std::make_shared<Graph>(instance_file, params_file);

    if(solver == "bp") {
        solve_bp(g);
    } else if(solver == "campelo") {
        solve_campelo(g);
    } else if(solver == "compact") {
        solve_compact(g);
    } else if(solver == "greedy") {
        solve_greedy(g);
    } else if(solver == "alns") {
        solve_alns(g, false);
    } else if(solver == "alns-stats") {
        solve_alns(g, true);
    } else if(solver == "tabu") {
        solve_tabu(g);
    } else if(solver == "grasp") {
        solve_grasp(g);
    } else if(solver == "decomposition") {
        solve_decomposition(g);
    } else if(solver == "info") {
        std::cout << g->data_filename << "," << g->n_vertices << "," << g->n_edges << "," << g->n_partitions << std::endl;
    } else {
        assert(false);
    }

    return 0;
}
