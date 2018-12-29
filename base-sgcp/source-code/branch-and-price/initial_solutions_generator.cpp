#include "initial_solutions_generator.hpp"
#include "../heuristics/greedy_heuristic.hpp"
#include "../heuristics/tabu_search.hpp"
#include "../heuristics/grasp.hpp"
#include "../heuristics/alns/alns.hpp"
#include "../utils/console_colour.hpp"

#include <thread>

namespace sgcp {
    InitialSolution InitialSolutionsGenerator::generate_from_existing(ColumnPool& start_solution) {
        using namespace std::chrono;

        assert(std::all_of(start_solution.begin(), start_solution.end(), [] (const auto& ss) { return ss.is_valid(false); }));

        ColumnPool alns_wa_columns, alns_nd_columns, tabu_columns;
        std::thread alns_wa, alns_nd, tabu;

        tabu = std::thread(
            [this, &tabu_columns, &start_solution] () {
                TabuSearchSolver solver{g};
                tabu_columns = solver.solve(start_solution);
            }
        );

        alns_wa = std::thread(
            [this, &alns_wa_columns, &start_solution] () {
                ALNSSolver solver{g};
                solver.use_acceptance_criterion("worse_accept");
                auto solution = solver.solve(start_solution);

                for(auto c = 0u; c < solution.n_colours; ++c) {
                    alns_wa_columns.emplace_back(g, solution.colours[c]);
                }
            }
        );

        alns_nd = std::thread(
            [this, &alns_nd_columns, &start_solution] () {
                ALNSSolver solver{g};
                solver.use_acceptance_criterion("accept_non_deteriorating");
                auto solution = solver.solve(start_solution);

                for(auto c = 0u; c < solution.n_colours; ++c) {
                    alns_nd_columns.emplace_back(g, solution.colours[c]);
                }
            }
        );

        auto start_time = high_resolution_clock::now();

        tabu.join();
        alns_wa.join();
        alns_nd.join();

        auto end_time = high_resolution_clock::now();
        auto elapsed_time = duration_cast<duration<float>>(end_time - start_time).count();

        auto best_sol_size = std::min({
            tabu_columns.size(),
            alns_wa_columns.size(),
            alns_nd_columns.size()
         });

        ColumnPool initial_columns;
        std::vector<uint32_t> best_id;

        if(tabu_columns.size() == best_sol_size) {
            best_id = std::vector<uint32_t>(tabu_columns.size());
            std::iota(best_id.begin(), best_id.end(), 0u);
            initial_columns = tabu_columns;
            add_unique(initial_columns, start_solution);
            add_unique(initial_columns, alns_wa_columns);
            add_unique(initial_columns, alns_nd_columns);
        } else if(alns_wa_columns.size() == best_sol_size) {
            best_id = std::vector<uint32_t>(alns_wa_columns.size());
            std::iota(best_id.begin(), best_id.end(), 0u);
            initial_columns = alns_wa_columns;
            add_unique(initial_columns, start_solution);
            add_unique(initial_columns, tabu_columns);
            add_unique(initial_columns, alns_nd_columns);
        } else if(alns_nd_columns.size() == best_sol_size) {
            best_id = std::vector<uint32_t>(alns_nd_columns.size());
            std::iota(best_id.begin(), best_id.end(), 0u);
            initial_columns = alns_nd_columns;
            add_unique(initial_columns, start_solution);
            add_unique(initial_columns, tabu_columns);
            add_unique(initial_columns, alns_wa_columns);
        } else {
            assert(false);
        }

        return InitialSolution{initial_columns, best_id, elapsed_time};
    }

    InitialSolution InitialSolutionsGenerator::generate() {
        using namespace Console;

        std::cout << colour_magenta("Obtaining initial solution with greedy heuristic") << std::endl;

        GreedyHeuristicSolver gs{g};
        auto start_solution = gs.solve();

        return generate_from_existing(start_solution);
    }

    void InitialSolutionsGenerator::add_unique(ColumnPool& pool, const ColumnPool& add) const {
        for(const auto& newcol : add) {
            auto it = std::find(pool.begin(), pool.end(), newcol);
            if(it == pool.end()) { pool.push_back(newcol); }
        }
    }
}
