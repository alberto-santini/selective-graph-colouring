#include "tabu_search.hpp"
#include "greedy_heuristic.hpp"

#include <random>
#include <chrono>

namespace sgcp {
    uint32_t TabuSearchSolver::smallest_set(const StableSetCollection& s) {
        assert(!s.empty());
        uint32_t si = 0u;
        uint32_t sz = s[si].size();
        
        for(auto i = 1u; i < s.size(); i++) { if(s[i].size() < sz) { si = i; } }
        
        return si;
    }
    
    void TabuSearchSolver::initialise(StableSetCollection initial_solution) {
        solutions = {initial_solution};
        target_colours_n = initial_solution.size() - 1;
        
        colouring_stable_sets = StableSetCollection{};
        
        uint32_t smallest_idx = smallest_set(initial_solution);
        std::vector<uint32_t> coloured_vertices;
        for(auto i = 0u; i < initial_solution.size(); i++) {
            if(i != smallest_idx) {
                colouring_stable_sets.push_back(initial_solution[i]);
                coloured_vertices.insert(coloured_vertices.end(), initial_solution[i].get_set().begin(), initial_solution[i].get_set().end());
            }
        }
        assert(colouring_stable_sets.size() == target_colours_n);
        
        uncoloured_set = VertexIdSet{};
        partitions_colour_status.coloured = std::set<uint32_t>{};
        partitions_colour_status.uncoloured = std::set<uint32_t>{};
        
        for(auto it = vertices(g.g); it.first != it.second; ++it.first) {
            auto id = g.g[*it.first].id;
            auto k = g.partition_for(id);
            if(std::find(coloured_vertices.begin(), coloured_vertices.end(), id) == coloured_vertices.end()) {
                uncoloured_set.insert(id);
            } else {
                partitions_colour_status.coloured.insert(k);
            }
        }
        
        for(auto k = 0u; k < g.n_partitions; k++) {
            if(std::find(
                partitions_colour_status.coloured.begin(),
                partitions_colour_status.coloured.end(),
                k
            ) == partitions_colour_status.coloured.end()) {
                partitions_colour_status.uncoloured.insert(k);
            }
        }
        
        iteration_n = 0;
        
        tabu_list = std::map<TabuElement, uint32_t>{};
    }
    
    uint32_t TabuSearchSolver::random_uncoloured_partition(std::mt19937& mt) const {
        assert(partitions_colour_status.uncoloured.size() > 0u);
        
        std::uniform_int_distribution<uint32_t> d(0, partitions_colour_status.uncoloured.size() - 1);
        auto it = partitions_colour_status.uncoloured.begin();
        std::advance(it, d(mt));
        return *it;
    }
    
    uint32_t TabuSearchSolver::random_uncoloured_vertex(uint32_t partition, std::mt19937& mt) const {
        assert(g.p[partition].size() > 0u);
        
        std::uniform_int_distribution<uint32_t> d(0, g.p[partition].size() - 1);
        auto it = g.p[partition].begin();
        std::advance(it, d(mt));
        return *it;
    }
    
    bool TabuSearchSolver::is_colourable(uint32_t vertex) const {
        auto times_in_tabu = 0u;
        for(const auto& el_it : tabu_list) {
            if(el_it.first.vertex_id == vertex) { times_in_tabu++; }
        }
        
        return (times_in_tabu != target_colours_n);
    }
    
    uint32_t TabuSearchSolver::external_degree(Vertex vertex, uint32_t partition) const {
        uint32_t degree = 0u;
        
        for(auto it = out_edges(vertex, g.g); it.first != it.second; ++it.first) {
            auto w = target(*it.first, g.g);
            auto j = g.partition_for(g.g[w].id);
            
            if(j != partition) { degree++; }
        }
        
        return degree;
    }
    
    TabuSearchSolver::InsertionResult TabuSearchSolver::simulate_insertion(uint32_t vertex, uint32_t partition, uint32_t colour) const {
        assert(colour < target_colours_n);
        
        InsertionResult r;
        
        r.inserted_vertex = vertex;
        r.coloured_partition = partition;
        r.colour = colour;
        r.removed_vertices = std::vector<uint32_t>{};
        r.uncoloured_partitions = std::set<uint32_t>{};
        
        // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
        boost::optional<uint32_t> score = boost::make_optional(false, 0u);
        
        auto v = g.vertex_by_id(vertex);
        assert(v);
        
        for(auto w_id : colouring_stable_sets[colour].get_set()) {
            auto w = g.vertex_by_id(w_id);
            assert(w);
            if(edge(*v, *w, g.g).second) {
                auto k = g.partition_for(w_id);
                r.removed_vertices.push_back(w_id);
                r.uncoloured_partitions.insert(k);
                
                if(!score) { score = external_degree(*w, k); }
                else {
                    if(g.params.tabu_score == "sum") {
                        score = *score + external_degree(*w, k);
                    } else {
                        score = std::min(*score, external_degree(*w, k));
                    }
                }
            }
        }
        
        r.score = (score ? *score : 0u);
        
        return r;
    }
    
    bool TabuSearchSolver::all_partitions_coloured() const {
        return partitions_colour_status.uncoloured.empty();
    }
    
    void TabuSearchSolver::insert(const TabuSearchSolver::InsertionResult& r, uint32_t tenure) {
        for(auto v : r.removed_vertices) {
            colouring_stable_sets[r.colour].remove_vertex(v);
            uncoloured_set.insert(v);
        }
        
        for(auto k : r.uncoloured_partitions) {
            partitions_colour_status.coloured.erase(k);
            partitions_colour_status.uncoloured.insert(k);
        }
        
        uncoloured_set.erase(r.inserted_vertex);
        colouring_stable_sets[r.colour].add_vertex(r.inserted_vertex);
        
        partitions_colour_status.uncoloured.erase(r.coloured_partition);
        partitions_colour_status.coloured.insert(r.coloured_partition);
        
        tabu_list.insert(std::make_pair(TabuElement{r.colour, r.inserted_vertex}, iteration_n + tenure));
        
        if(all_partitions_coloured()) { solutions.push_back(colouring_stable_sets); }
    }

    StableSetCollection TabuSearchSolver::solve(boost::optional<StableSetCollection> initial_solution, float* elapsed_time) {
        using namespace std::chrono;

        if(!initial_solution) {
            GreedyHeuristicSolver gs{g};
            initial_solution = gs.solve();
        }

        auto stime = high_resolution_clock::now();
        while(true) {
            auto tabu_sol = solve_iter(*initial_solution);
            if(tabu_sol.size() == 1u) {
                auto etime = high_resolution_clock::now();

                if(elapsed_time) {
                    *elapsed_time = duration_cast<duration<float>>(etime - stime).count();
                }

                return tabu_sol.back();
            }
            initial_solution = tabu_sol.back();
        }
    }

    std::vector<StableSetCollection> TabuSearchSolver::solve_iter(const StableSetCollection& initial_solution) {
        // Cannot colour with fewer than 1 colour, can we?
        if(initial_solution.size() == 1u) { return {initial_solution}; }
        
        initialise(initial_solution);
        
        std::mt19937::result_type random_data[std::mt19937::state_size];
        std::random_device source;
        std::generate(std::begin(random_data), std::end(random_data), std::ref(source));
        std::seed_seq seeds(std::begin(random_data), std::end(random_data));
        std::mt19937 mt(seeds);

        std::uniform_int_distribution<uint32_t> tenure_dist(g.params.tabu_min_rnd_tenure, g.params.tabu_max_rnd_tenure);

        uint32_t max_iterations = g.params.tabu_iterations;

        if(g.params.tabu_instance_scaled_iters) {
            max_iterations = static_cast<uint32_t>(
                std::round(
                    static_cast<float>(max_iterations) / std::log2(static_cast<float>(g.n_vertices))
                )
            );
        }
        
        while(iteration_n < max_iterations) {
            auto k = random_uncoloured_partition(mt);
            auto v = random_uncoloured_vertex(k, mt);
            
            if(!is_colourable(v)) { return solutions; }
            
            std::map<uint32_t, InsertionResult> scores;
            
            for(auto c = 0u; c < target_colours_n; c++) {
                auto tabu_it = std::find_if(
                    tabu_list.begin(),
                    tabu_list.end(),
                    [&] (const auto& el_it) {
                        return el_it.first.colour == c && el_it.first.vertex_id == v;
                    }
                );
                if(tabu_it == tabu_list.end()) { scores[c] = simulate_insertion(v, k, c); }
            }
            
            auto best_insertion_it = std::min_element(
                scores.begin(),
                scores.end(),
                [&] (const auto& col_ins_1, const auto& col_ins_2) {
                    return col_ins_1.second.score < col_ins_2.second.score;
                }
            );

            uint32_t tenure = g.params.tabu_tenure;
            if(g.params.tabu_randomised_tenure) {
                tenure = tenure_dist(mt);
            }
            
            insert(best_insertion_it->second, tenure);
            
            if(all_partitions_coloured()) { return solutions; }
            else { update_tabu_list(); }
            
            iteration_n++;
        }
        
        return solutions;
    }
    
    void TabuSearchSolver::update_tabu_list() {
        auto it = tabu_list.begin();
        auto end_it = tabu_list.end();
        
        while(it != end_it) {
            if(it->second == iteration_n) { tabu_list.erase(it++); }
            else { ++it; }
        }
    }
}