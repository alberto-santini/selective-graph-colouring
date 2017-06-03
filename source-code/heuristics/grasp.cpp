//
// Created by alberto on 11/03/17.
//

#include <mwss/sewell_mwss_solver.hpp>
#include <mutex>
#include <thread>
#include <heuristics/alns/local_search.hpp>
#include "grasp.hpp"

namespace sgcp {
    std::pair<Graph, WeightMap> GRASPSolver::reduce(const Graph& g, const WeightMap& wm, const std::set<uint32_t>& coloured_v) const {
        using namespace boost;

        BoostGraph new_bg;
        Partition new_p;
        WeightMap new_wm;
        std::map<uint32_t, uint32_t> id_shift;
        auto new_v_idx = 0u;

        for(auto it = vertices(g.g); it.first != it.second; ++it.first) {
            Vertex v = *it.first;
            uint32_t v_id = g.g[v].id;

            if(std::find(coloured_v.begin(), coloured_v.end(), v_id) == coloured_v.end()) {
                Vertex new_v = add_vertex(new_bg);
                new_bg[new_v] = VertexInfo{new_v_idx, g.g[v].represented_vertices};
                id_shift[v_id] = new_v_idx;
                new_wm[new_v_idx] = wm.at(v_id);
                ++new_v_idx;
            }
        }

        for(auto it = edges(g.g); it.first != it.second; ++it.first) {
            Edge e = *it.first;
            Vertex v_from = source(e, g.g);
            Vertex v_to = target(e, g.g);
            uint32_t from_id = g.g[v_from].id;
            uint32_t to_id = g.g[v_to].id;

            if( std::find(coloured_v.begin(), coloured_v.end(), from_id) == coloured_v.end() &&
                std::find(coloured_v.begin(), coloured_v.end(), to_id) == coloured_v.end())
            {
                // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
                // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
                auto new_v_from = boost::make_optional(false, Vertex{});
                auto new_v_to = boost::make_optional(false, Vertex{});

                for(auto vit = vertices(new_bg); vit.first != vit.second; ++vit.first) {
                    if(!new_v_from && new_bg[*vit.first].id == id_shift.at(from_id)) { new_v_from = *vit.first; if(new_v_to) { break; } }
                    if(!new_v_to && new_bg[*vit.first].id == id_shift.at(to_id)) { new_v_to = *vit.first; if(new_v_from) { break; } }
                }
                assert(new_v_from && new_v_to);

                add_edge(*new_v_from, *new_v_to, new_bg);
            }
        }

        for(auto k = 0u; k < g.n_partitions; k++) {
            auto new_set = std::unordered_set<uint32_t>();
            for(auto id : g.p[k]) {
                if(std::find(coloured_v.begin(), coloured_v.end(), id) == coloured_v.end()) {
                    auto vit = vertices(new_bg);
                    auto new_v = std::find_if(vit.first, vit.second, [&] (auto v) { return new_bg[v].id == id_shift.at(id); });

                    assert(new_v != vit.second);

                    new_set.insert(*new_v);
                }
            }
            new_p.push_back(new_set);
        }

        return std::make_pair(Graph{new_bg, new_p, g.params}, new_wm);
    }

    ColumnPool GRASPSolver::greedy_mwss_solve(WeightMap weights) const {
        std::set<uint32_t> coloured_v;
        ColumnPool cp;

        while(coloured_v.size() < g.n_vertices) {
            auto red = reduce(g, weights, coloured_v);
            const auto& red_graph = red.first;
            const auto& red_weights = red.second;

            auto solver = SewellMwssSolver{g, red_graph, red_weights};
            auto solution = solver.solve();

            assert(solution);

            cp.push_back(*solution);

            for(auto v_id : solution->get_set()) {
                auto v = g.vertex_by_id(v_id);
                assert(v);

                for(auto w : g.g[*v].represented_vertices) {
                    auto w_id = g.g[w].id;
                    auto k = g.partition_for(w_id);

                    for(auto x_id : g.p[k]) {
                        __attribute__((unused)) auto it_b = coloured_v.insert(x_id);
                        assert(it_b.second);
                    }
                }
            }
        }

        return cp;
    }

    WeightMap GRASPSolver::make_random_weight_map() const {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<uint32_t> dist(0, g.n_vertices);
        WeightMap wm;

        for(auto i = 0u; i < g.n_vertices; ++i) {
            wm[i] = dist(mt);
        }

        return wm;
    }

    ColumnPool GRASPSolver::solve() const {
        boost::optional<ColumnPool> cp;
        std::mutex cp_mtx;
        auto iter = 0u;

        while(iter < g.params.grasp_iterations) {
            std::vector<std::thread> threads;

            for(auto i = 0u; i < g.params.grasp_threads; ++i) {
                threads.emplace_back([&] () noexcept {
                    WeightMap wm = this->make_random_weight_map();
                    ColumnPool sol = this->greedy_mwss_solve(wm);
                    DecreaseByOneColourLocalSearch ls{g};
                    ALNSColouring col{g, sol};

                    auto old_sz = col.n_colours;
                    while(true) {
                        col = ls.attempt_local_search(col);
                        if(col.n_colours < old_sz) {
                            old_sz = col.n_colours;
                        } else {
                            break;
                        }
                    }

                    sol = col.to_column_pool();

                    std::lock_guard<std::mutex> guard(cp_mtx);
                    if(!cp || sol.size() < cp->size()) {
                        cp = sol;
                    }
                });
            }

            for(auto& t : threads) { t.join(); }
            iter += g.params.grasp_threads;
        }

        assert(cp);

        return *cp;
    }
}