#include "local_search.hpp"
#include "../../stable_set.hpp"

#include <vector>
#include <iostream>

namespace sgcp {
    ALNSColouring DecreaseByOneColourLocalSearch::attempt_local_search(const ALNSColouring& c) const {
        ALNSColouring n = c;

        uint32_t empty_col_id = n.n_colours;
        uint32_t empty_col_sz = g.n_partitions + 1;
        for(auto i = 0u; i < n.n_colours; i++) {
            auto sz = n.colours[i].size();
            if(sz < empty_col_sz) {
                empty_col_id = i;
                empty_col_sz = sz;
            }
        }
        assert(empty_col_id < n.n_colours);
        assert(empty_col_sz < g.n_partitions + 1);

        auto empty_me = n.colours[empty_col_id];
        for(auto v : empty_me) { n.uncolour_vertex(v); }

        auto colour_me = n.uncoloured_partitions;
        for(auto p : colour_me) { try_to_colour(n, p); }

        return (n.n_colours < c.n_colours) ? n : c;
    }

    void DecreaseByOneColourLocalSearch::try_to_colour(ALNSColouring& n, uint32_t p) const {
        assert(n.is_valid());

        // Tries to colour any vertex of cluster p

        // For each vertex v of cluster p
        for(auto v : g.p[p]) {
            // Try to place it in any colour i
            for(auto i = 0u; i < n.n_colours; ++i) {
                // If v were to enter colour i, then all these partitions would be uncoloured
                auto u = partitions_not_compatible_with(n, i, v);
                bool managed = true;

                // For each partition that should be uncoloured
                for(auto q : u) {
                    // If I can simply colour that partition with another colour (without uncolouring any other vertex), ok.
                    if(try_to_recolour(n, i, q, v)) { continue; }
                    // Otherwise, I can try to colour that partition with another colour, and further move/uncolour vertices incompatible with q.
                    if(try_to_move(n, i, q)) { continue; }
                    // If I can't even do that, then I give up on partition q =>
                    // I give up on trying to colour vertex v with colour i =>
                    // I will try with the next colour (if any), or the next vertex of p (otherwise)
                    managed = false;

                    assert(n.is_valid());

                    break;
                }

                // I removed all vertices incompatible with v from colour i
                if(managed) {
                    // We have to check whether colour i got completely emptied. In this case,
                    // colour indices would have "shifted" and colour i would have been replaced by
                    // another colour. In this last case, we need to try with next colour.
                    // In short, we only put v in i if the set of partitions coloured by i which would
                    // not be compatible with v, is empty.
                    auto unew = partitions_not_compatible_with(n, i, v);
                    if(unew.empty()) { n.colour_vertex(v, i); return; } else { --i; continue; }
                }
            }
        }

        // If I could not manage to move v anywhere, I create a new colour for it.
        auto any_v = *(g.p[p].begin());
        n.colour_vertex(any_v, n.n_colours);

        assert(n.is_valid());
    }

    std::vector<uint32_t> DecreaseByOneColourLocalSearch::partitions_not_compatible_with(const ALNSColouring& n, uint32_t i, uint32_t v) const {
        std::vector<uint32_t> nc;

        for(auto w : n.colours[i]) {
            if(g.connected(v, w)) {
                nc.push_back(n.partition_for.at(w));
            }
        }
        return nc;
    }

    bool DecreaseByOneColourLocalSearch::try_to_recolour(ALNSColouring& n, uint32_t i, uint32_t q, uint32_t v) const {
        assert(n.is_valid());

        for(auto otherv : g.p[q]) {
            if(
                !g.connected(otherv, v) &&
                std::all_of(n.colours[i].begin(), n.colours[i].end(),
                    [&] (auto vi) {
                        return vi == otherv || !g.connected(otherv, vi);
                    }
                )
            ) {
                n.uncolour_partition(q);
                n.colour_vertex(otherv, i);
                return true;
            }
        }

        assert(n.is_valid());
        return false;
    }

    bool DecreaseByOneColourLocalSearch::try_to_move(ALNSColouring& n, uint32_t i, uint32_t q) const {
        assert(n.is_valid());

        // Move partition q away from colour i
        bool moved = false;

        for(auto j = 0u; j < n.n_colours; j++) {
            if(j == i) { continue; }

            // Trying to colour q with colour j != i
            for(auto v : g.p[q]) {
                // Try to colour q's vertex v with colour j != i

                // Get the list of partitions incompatible with placing v in j
                auto u = partitions_not_compatible_with(n, j, v);

                if(u.empty()) {
                    size_t size_of_colour_i = n.colours[i].size();

                    // If no partition is incompatible, uncolour whatever vertex was coloured in q,
                    // and colour vertex v (of q) with colour j
                    assert(n.is_valid());
                    n.uncolour_partition(q);
                    assert(n.is_valid());

                    if(size_of_colour_i == 1u && i < j) {
                        // By uncolouring q, I emptied colour i.
                        // Therefore, all colours > i are now decreased by 1.
                        // Since j > i, j is also decreased by 1.
                        --j;
                    }

                    n.colour_vertex(v, j);
                    assert(n.is_valid());
                    moved = true;
                    break;
                }
            }

            if(moved) { break; }
        }

        assert(n.is_valid());
        return moved;
    }
}
