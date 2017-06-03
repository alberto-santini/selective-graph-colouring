#include "alns.hpp"
#include "../greedy_heuristic.hpp"

#include <chrono>
#include <numeric>

namespace sgcp {
    ALNSSolver::ALNSSolver(const Graph& g) : g{g}, acceptance{nullptr}, local_search{nullptr} {
        // Seed the pseudo-random number generator.
        std::mt19937::result_type random_data[std::mt19937::state_size];
        std::random_device source;
        std::generate(std::begin(random_data), std::end(random_data), std::ref(source));
        std::seed_seq seeds(std::begin(random_data), std::end(random_data));
        mt = std::mt19937(seeds);

        if(g.params.alns_instance_scaled_iters) {
            max_iterations = static_cast<uint32_t>(
                std::round(
                    static_cast<float>(g.params.alns_iterations) / std::log2(static_cast<float>(g.n_vertices))
                )
            );
        } else {
            max_iterations = g.params.alns_iterations;
        }

        use_acceptance_criterion(g.params.alns_acceptance);
        acceptance->mt = &mt;

        if(g.params.alns_local_search == "decrease_by_one") {
            local_search = new DecreaseByOneColourLocalSearch{g};
        } else if(g.params.alns_local_search == "none") {
            local_search = nullptr;
        } else {
            throw "Local search operator not recognised!";
        }

        tabu_tenure = g.params.tabu_tenure;

        auto dmoves = initialise_destroy_moves();
        assert(dmoves.size() == g.params.alns_dmoves.size());

        for(auto i = 0u; i < dmoves.size(); ++i) {
            if(g.params.alns_dmoves[i] == 1u) {
                destroy.push_back(std::move(dmoves[i]));
                destroy_score.push_back(1.0);
            } else {
                dmoves[i].reset();
            }
        }

        auto rmoves = initialise_repair_moves();
        assert(rmoves.size() == g.params.alns_rmoves.size());

        for(auto i = 0u; i < rmoves.size(); ++i) {
            if(g.params.alns_rmoves[i] == 1u) {
                repair.push_back(std::move(rmoves[i]));
                repair_score.push_back(1.0);
            } else {
                rmoves[i].reset();
            }
        }
    }

    void ALNSSolver::use_acceptance_criterion(const std::string& ac_description) {
        if(acceptance) {
            delete acceptance;
            acceptance = nullptr;
        }

        if(g.params.alns_acceptance == "accept_everything") {
            acceptance = new AcceptEverything{};
        } else if(g.params.alns_acceptance == "accept_non_deteriorating") {
            acceptance = new AcceptNonDeteriorating{};
        } else if(g.params.alns_acceptance == "worse_accept") {
            acceptance = new WorseAccept{g.params.alns_wa_initial_probability, max_iterations};
        } else {
            throw "Acceptance criterion not recognised!";
        }
    }

    ALNSColouring ALNSSolver::solve(boost::optional<StableSetCollection> initial, float* elapsed_time) {
        // Iteration counter.
        uint32_t current_iteration = 0;

        // Current solution.
        ALNSColouring current = initial ? initial_solution(*initial) : initial_solution();

        // Best solution.
        ALNSColouring best = current;

        using namespace std::chrono;
        auto start_time = high_resolution_clock::now();

        // While the end criterion is not met:
        while(current_iteration++ < max_iterations) {
            // Cannot colour with fewer than 1 colour, can we?
            if(current.n_colours == 1u) { return current; }
            
            // Initialise the incumbent as the current, for the moment.
            // It will soon be modified with destroy and repair methods.
            ALNSColouring incumbent = current;

            // Select a destroy and repair method with roulette wheel
            // selection.
            auto destroy_id = roulette_wheel(destroy_score);
            auto repair_id = roulette_wheel(repair_score);

            // Apply the destroy and repair methods, to obtain a new
            // solution.
            (*destroy[destroy_id])(incumbent);
            (*repair[repair_id])(incumbent, tabu_list, current_iteration);

            // Apply the local search operator, if available.
            if(local_search != nullptr) { incumbent = local_search->attempt_local_search(incumbent); }

            // If the new solution is accepted:
            if((*acceptance)(current, incumbent, current_iteration)) {
                // If the new solution improves on the best currently known:
                if(incumbent.score() < best.score()) {
                    // Update best and scores accordingly.
                    best = incumbent;
                    update_score_found_best(destroy_id, destroy_score);
                    update_score_found_best(repair_id, repair_score);
                } else if(incumbent.score() < current.score()) {
                    // If the new solution only improves on the current:
                    // Update the scores accordingly.
                    update_score_found_better(destroy_id, destroy_score);
                    update_score_found_better(repair_id, repair_score);
                }

                // In any case, since the new move was accepted, it will
                // replace the current.
                current = incumbent;
            } else if(incumbent.score() > current.score()) {
                // If the new move was not accepted and it actually has
                // a worse score than the current:
                // Update the scores accordingly.
                update_score_found_worse(destroy_id, destroy_score);
                update_score_found_worse(repair_id, repair_score);
            }

            // Check if there is any move that should get out of the tabu list.
            clean_up_tabu_list(current_iteration);
        }

        auto end_time = high_resolution_clock::now();

        // If a pointer to store the elapsed time was passed, save it there.
        if(elapsed_time != nullptr) {
            *elapsed_time = duration_cast<duration<float>>(end_time - start_time).count();
        }

        // Return the best solution encountered.
        return best;
    }

    std::vector<std::unique_ptr<DestroyMove>> ALNSSolver::initialise_destroy_moves() const {
        std::vector<std::unique_ptr<DestroyMove>> d;

        d.emplace_back(new RemoveRandomVertexInRandomColour{g, mt});
        d.emplace_back(new RemoveRandomVertexInSmallestColour{g, mt});
        d.emplace_back(new RemoveVertexWithSmallestDegree{g, mt});
        d.emplace_back(new RemoveVertexWithSmallestColourDegree{g, mt});
        d.emplace_back(new RemoveVertexByRouletteDegreeSmall{g, mt});
        d.emplace_back(new RemoveVertexByRouletteColourDegreeSmall{g, mt});
        d.emplace_back(new RemoveRandomColour{g, mt});
        d.emplace_back(new RemoveSmallestColour{g, mt});
        d.emplace_back(new RemoveColourWithSmallestDegree{g, mt});
        d.emplace_back(new RemoveColourWithSmallestColourDegree{g, mt});
        d.emplace_back(new RemoveColourByRouletteDegreeSmall{g, mt});
        d.emplace_back(new RemoveColourByRouletteColourDegreeSmall{g, mt});
        d.emplace_back(new RemoveRandomVertexInBiggestColour{g, mt});
        d.emplace_back(new RemoveVertexWithBiggestDegree{g, mt});
        d.emplace_back(new RemoveVertexWithBiggestColourDegree{g, mt});
        d.emplace_back(new RemoveVertexByRouletteDegreeBig{g, mt});
        d.emplace_back(new RemoveVertexByRouletteColourDegreeBig{g, mt});

        return d;
    }

    std::vector<std::unique_ptr<RepairMove>> ALNSSolver::initialise_repair_moves() const {
        std::vector<std::unique_ptr<RepairMove>> r;

        r.emplace_back(new InsertRandomVertexInRandomColour{g, mt});
        r.emplace_back(new InsertRandomVertexInBiggestColour{g, mt});
        r.emplace_back(new InsertRandomVertexInSmallestColour{g, mt});
        r.emplace_back(new InsertLowestDegreeVertexInRandomColour{g, mt});
        r.emplace_back(new InsertLowestDegreeVertexInBiggestColour{g, mt});
        r.emplace_back(new InsertLowestDegreeVertexInSmallestColour{g, mt});
        r.emplace_back(new InsertLowestColourDegreeVertexInRandomColour{g, mt});
        r.emplace_back(new InsertLowestColourDegreeVertexInBiggestColour{g, mt});
        r.emplace_back(new InsertLowestColourDegreeVertexInSmallestColour{g, mt});

        return r;
    }

    ALNSColouring ALNSSolver::initial_solution(const StableSetCollection& pool) const {
        ALNSColouring initial{g};

        // Colour the vertices one by one.
        uint32_t colour_id = 0u;
        for(const auto& s : pool) {
            if(s.dummy) { continue; }

            for(const auto& v : s.get_set()) {
                auto p = initial.partition_for[v];

                // Do not colour the same partition twice, even if a heuristic initial solution
                // might have two colours which colour the same partition.
                if(
                    std::find(initial.coloured_partitions.begin(), initial.coloured_partitions.end(), p) !=
                    initial.coloured_partitions.end()
                ) { continue; }

                initial.colour_vertex(v, colour_id);
            }

            colour_id++;
        }

        return initial;
    }

    ALNSColouring ALNSSolver::initial_solution() const {
        // Obtain two solutions from the two greedy heuristics.
        GreedyHeuristicSolver gs{g};
        return initial_solution(gs.solve());
    }

    uint32_t ALNSSolver::roulette_wheel(const std::vector<float>& vec) const {
        // Performs a roulette wheel selection: it return the index of an element i of vec,
        // with probability directly proportional to its score vec[i]. It assumes that
        // all scores are non-negative.

        float scores_sum = std::accumulate(vec.begin(), vec.end(), 0.0);
        std::uniform_real_distribution<float> dis(0, scores_sum);
        float score_rnd = dis(mt);
        float scores_acc = 0.0;

        for(auto i = 0u; i < vec.size(); i++) {
            scores_acc += vec[i];
            if(score_rnd <= scores_acc) { return i; }
        }

        return vec.size() - 1;
    }

    void ALNSSolver::update_score_found_best(uint32_t i, std::vector<float>& vec) const {
        // When the method gives us a new best, increase its score by x%.
        vec[i] *= g.params.alns_new_best_mult;
    }

    void ALNSSolver::update_score_found_better(uint32_t i, std::vector<float>& vec) const {
        // When the method gives us a new improving solution, increase its score by y%.
        vec[i] *= g.params.alns_new_improving_mult;
    }

    void ALNSSolver::update_score_found_worse(uint32_t i, std::vector<float>& vec) const {
        // When the method gives us a new solution worse than the current, decrease its score by z%.
        vec[i] *= g.params.alns_worsening_mult;
    }

    void ALNSSolver::clean_up_tabu_list(uint32_t current_iteration) {
        // Erase elements from the tabu list if their tenure is expired.
        for(auto it = tabu_list.begin(); it != tabu_list.end();) {
            it->second.erase(
                std::remove_if(
                    it->second.begin(),
                    it->second.end(),
                    [&] (const auto& tm) { return tm.entry_iteration + tabu_tenure < current_iteration; }
                ),
                it->second.end()
            );

            if(it->second.empty()) {
                it = tabu_list.erase(it);
            } else {
                ++it;
            }
        }
    }

    ALNSSolver::~ALNSSolver() {
        if(acceptance) { delete acceptance; acceptance = nullptr; }
        if(local_search) { delete local_search; local_search = nullptr; }
    }
}