#include "greedy_heuristic.hpp"
#include "../utils/cache.hpp"

#include <boost/optional.hpp>
#include <vector>
#include <numeric>

namespace sgcp {
    StableSetCollection GreedyHeuristicSolver::solve() const {
        auto pool = ColumnPool{};
        cache::init_update_pool(pool, g);

        if(!pool.empty()) { return pool; }

        auto simple_pool = solve_simple();
        auto improved_pool = solve_improved();

        // Get the best of the two solutions.
        if(simple_pool.size() < improved_pool.size()) {
            cache::init_update_cache(simple_pool, g);
            return simple_pool;
        } else {
            cache::init_update_cache(improved_pool, g);
            return improved_pool;
        }
    }

    StableSetCollection GreedyHeuristicSolver::solve(bool improved) const {
        StableSetCollection sol;
        std::vector<uint32_t> uncoloured(g.n_partitions);
        std::iota(uncoloured.begin(), uncoloured.end(), 0);

        sol.emplace_back(g, VertexIdSet{});

        while(!uncoloured.empty()) {
            // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
            boost::optional<uint32_t> p_id = boost::make_optional(false, 0u);
            boost::optional<uint32_t> any_p_id = boost::make_optional(false, 0u);
            boost::optional<uint32_t> v_id = boost::make_optional(false, 0u);
            boost::optional<uint32_t> any_v_id = boost::make_optional(false, 0u);
            boost::optional<uint32_t> v_degree = boost::make_optional(false, 0u);
            boost::optional<uint32_t> any_v_degree = boost::make_optional(false, 0u);

            for(auto k : uncoloured) {
                // Trying to find a vertex in p that can be added to the
                // stable set. We prefer inserting the vertex with the
                // lowest degree.
                for(auto w_id : g.p[k]) {
                    // Check if it can be added to the stable set, i.e. if
                    // it is not connected to some other vertex in the set.
                    bool addable = std::none_of(
                        sol.back().get_set().begin(),
                        sol.back().get_set().end(),
                        [&] (auto sv_id) {
                            return g.connected(sv_id, w_id);
                        }
                    );

                    auto w = g.vertex_by_id(w_id);
                    assert(w);

                    uint32_t out_d = 0u;

                    if(improved) {
                        for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                            auto z = target(*it.first, g.g);
                            auto zk = g.partition_for(g.g[z].id);
                            if(std::find(uncoloured.begin(), uncoloured.end(), zk) != uncoloured.end()) { out_d++; }
                        }
                    } else {
                        out_d = out_degree(*w, g.g);
                    }
                    
                    // Update the ids of the best vertex to be added to
                    // the current stable set (if addable == true) or of
                    // the best candidate to start a new stable set (if
                    // addable == false).
                    if(addable && (!v_degree || out_d < *v_degree)) {
                        v_id = w_id;
                        v_degree = out_d;
                        p_id = k;
                    }
                    if(!any_v_degree || out_d < *any_v_degree) {
                        any_v_id = w_id;
                        any_v_degree = out_d;
                        any_p_id = k;
                    }
                }
            }

            if(v_id) {
                assert(p_id);

                // Add vertex to current stable set
                sol.back().add_vertex(*v_id);
                // Mark the partition as coloured
                uncoloured.erase(std::remove(uncoloured.begin(), uncoloured.end(), *p_id), uncoloured.end());
            } else {
                assert(any_p_id);

                // Add vertex to a new stable set
                sol.emplace_back(g, VertexIdSet{*any_v_id});
                // Mark the partition as coloured
                uncoloured.erase(std::remove(uncoloured.begin(), uncoloured.end(), *any_p_id), uncoloured.end());
            }
        }

        return sol;
    }
}