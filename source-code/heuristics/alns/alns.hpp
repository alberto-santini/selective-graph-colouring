#ifndef _ALNS_HPP
#define _ALNS_HPP

#include "../../graph.hpp"
#include "../../branch-and-price/column_pool.hpp"
#include "alns_colouring.hpp"
#include "tabu_list.hpp"
#include "destroy.hpp"
#include "repair.hpp"
#include "acceptance.hpp"
#include "local_search.hpp"

#include <vector>
#include <random>

namespace sgcp {
    // This class represents the solver for the ALNS algorithm.
    struct ALNSSolver {
        // Reference to original graph.
        const Graph& g;

        // Mersenne Twister for PRN generation.
        mutable std::mt19937 mt;

        // Maximum number of iterations
        uint32_t max_iterations;

        // Acceptance criterion to use.
        ALNSAcceptance* acceptance;

        // (Eventual) local search operator to improve the solution
        // after destroy and repair.
        LocalSearchOperator* local_search;

        // List of destroy moves.
        std::vector<std::unique_ptr<DestroyMove>> destroy;

        // List of scores of destroy moves.
        std::vector<float> destroy_score;

        // List of repair moves.
        std::vector<std::unique_ptr<RepairMove>> repair;

        // List of scores of repair moves.
        std::vector<float> repair_score;

        // To avoid that the repair move simply moves back a removed vertex
        // in the place it was before being destroyed, we keep a short-tenured
        // tabu list. tabu_list[v] contains all tabu moves relative to
        // vertex v.
        TabuList tabu_list;

        // Specifies for how many iterations a rule should stay in the tabu list.
        uint32_t tabu_tenure;

        // Sets the appropriate acceptance criterion.
        void use_acceptance_criterion(const std::string& ac_description);

        // Initialises and returns all destroy moves.
        std::vector<std::unique_ptr<DestroyMove>> initialise_destroy_moves() const;

        // Initialises and returns all repair moves.
        std::vector<std::unique_ptr<RepairMove>> initialise_repair_moves() const;

        // Creates an initial solution, starting from a column pool.
        ALNSColouring initial_solution(const ColumnPool& pool) const;

        // Creates an initial solution, by first calling the constructive heuristics.
        ALNSColouring initial_solution() const;

        // Runs the ALNS for ``max_iterations'' iterations and returns the best colouring.
        // You can optionally pass an initial solution. If you don't (i.e. pass nullptr
        // as the second argument), the constructive heuristics are called to generate
        // the initial solution. You can also pass a pointer to a float where we will store
        // the elapsed time in seconds (this is optional).
        ALNSColouring solve(boost::optional<StableSetCollection> initial = boost::none, float* elapsed_time = nullptr);

        // Gives the index of an element of vec, selected according to roulette
        // wheel selection, where the probabilities are proportional to the values
        // contained in vec.
        uint32_t roulette_wheel(const std::vector<float>& vec) const;

        // Updates the score of element i of vector vec, when that element leads
        // to a new best.
        void update_score_found_best(uint32_t i, std::vector<float>& vec) const;

        // Updates the score of element i of vector vec, when that element leads
        // to a solution improving on the current (but which is not the new best).
        void update_score_found_better(uint32_t i, std::vector<float>& vec) const;

        // Updates the score of element i of vector vec, when that element leads
        // to a worsening solution.
        void update_score_found_worse(uint32_t i, std::vector<float>& vec) const;

        // Goes through the tabu list and removes expired entries.
        void clean_up_tabu_list(uint32_t current_iteration);

        // Builds an ALNS solver with a specified acceptance criterion and tabu tenure.
        // It also initialises the PRNG of the solver and passes it to the acceptance
        // criterion. If additional_moves is true, it will use more destroy and repair
        // moves.
        ALNSSolver(const Graph& g);

        // Cleans up the memory.
        ~ALNSSolver();
    };
}
#endif