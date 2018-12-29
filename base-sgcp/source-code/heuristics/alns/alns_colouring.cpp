#include "alns_colouring.hpp"

namespace sgcp {
    ALNSColouring::ALNSColouring(const Graph& g) : g{g}, n_colours{0} {
        id.reserve(g.n_partitions);
        colours.reserve(g.n_partitions);
        coloured_vertices.reserve(g.n_vertices);
        uncoloured_vertices.reserve(g.n_vertices);
        coloured_partitions.reserve(g.n_partitions);
        uncoloured_partitions.reserve(g.n_partitions);

        for(auto v = 0u; v < g.n_vertices; v++) {
            uncoloured_vertices.push_back(v);
            partition_for[v] = g.partition_for(v);
        }

        for(auto p = 0u; p < g.n_partitions; p++) {
            uncoloured_partitions.push_back(p);
        }
    }

    ALNSColouring::ALNSColouring(const Graph& g, const ColumnPool& cp) : ALNSColouring{g} {
        for(auto c_id = 0u; c_id < cp.size(); ++c_id) {
            for(auto v_id : cp[c_id].get_set()) {
                colour_vertex(v_id, c_id);
            }
        }

        assert(is_valid());
    }

    ColumnPool ALNSColouring::to_column_pool() const {
        ColumnPool cp;

        for(const auto& col : colours) {
            cp.emplace_back(g.get(), col);
        }

        return cp;
    }

    void ALNSColouring::uncolour_partition(uint32_t p) {
        assert(is_valid());
        assert(std::find(coloured_partitions.begin(), coloured_partitions.end(), p) != coloured_partitions.end());

        for(auto v : coloured_vertices) {
            if(partition_for[v] == p) {
                return uncolour_vertex(v);
            }
        }

        assert(false);
    }

    void ALNSColouring::uncolour_vertex(uint32_t v) {
        assert(is_valid());
        assert(std::find(coloured_vertices.begin(), coloured_vertices.end(), v) != coloured_vertices.end());
        assert(std::find(uncoloured_vertices.begin(), uncoloured_vertices.end(), v) == uncoloured_vertices.end());
        assert(std::find(coloured_partitions.begin(), coloured_partitions.end(), partition_for[v]) != coloured_partitions.end());
        assert(std::find(uncoloured_partitions.begin(), uncoloured_partitions.end(), partition_for[v]) == uncoloured_partitions.end());

        uint32_t c = coloured[v];

        // Remove v from the (coloured vertex) => colour mapping.
        coloured.erase(v);

        // Erase v from the list of vertices coloured with c.
        colours[c].erase(std::remove(colours[c].begin(), colours[c].end(), v), colours[c].end());

        // If v was the only vertex coloured with c, we remove colour c and
        // decrease the number of used colours.
        if(colours[c].empty()) {
            // Remove the (empty colour).
            colours.erase(colours.begin() + c);

            // Remove the corresponding id.
            id.erase(id.begin() + c);

            // Shift all the entries of ``coloured'' which come
            // after the removed colour.
            for(auto& vc : coloured) {
                if(vc.second > c) { vc.second--; }
            }

            // Decrease the number of colours used.
            n_colours--;

            assert(colours.size() == n_colours);
        }

        // We move v from the set of coloured to that of uncoloured vertices.
        coloured_vertices.erase(std::remove(coloured_vertices.begin(), coloured_vertices.end(), v), coloured_vertices.end());
        uncoloured_vertices.push_back(v);

        // WARNING! The following works under the assumption that for each coloured partition, there is at any point
        // only one vertex of the partition that is coloured. That is, the selective colouring is as tight as possible.

        // We move p from the set of coloured to that of uncoloured partitions.
        coloured_partitions.erase(std::remove(coloured_partitions.begin(), coloured_partitions.end(), partition_for[v]), coloured_partitions.end());
        uncoloured_partitions.push_back(partition_for[v]);

        assert(is_valid());
    }

    void ALNSColouring::colour_vertex(uint32_t v, uint32_t c) {
        assert(is_valid());
        assert(std::find(coloured_vertices.begin(), coloured_vertices.end(), v) == coloured_vertices.end());
        assert(std::find(uncoloured_vertices.begin(), uncoloured_vertices.end(), v) != uncoloured_vertices.end());
        assert(std::find(coloured_partitions.begin(), coloured_partitions.end(), partition_for[v]) == coloured_partitions.end());
        assert(std::find(uncoloured_partitions.begin(), uncoloured_partitions.end(), partition_for[v]) != uncoloured_partitions.end());

        // If we need to add a new colour:
        if(c == n_colours) {
            // Add a new colour with only v as its coloured vertex.
            colours.push_back({v});

            // Give the new colour a (new) id. It will be sufficient
            // to use the last id plus one: since colours can be removed
            // from anywhere (when they are emptied), but are always added
            // at the back, they will be in ascending order. In case
            // the colouring is empty, just use 0 as new id.
            id.push_back(id.empty() ? 0 : id.back() + 1);

            // Increase the number of used colours.
            n_colours++;

            assert(colours.size() == n_colours);
        } else {
            assert(std::none_of(colours[c].begin(), colours[c].end(),
                [this,&v] (auto w) { return g.get().connected(v, w); }
            ));

            // Add v to existing colour c:
            colours[c].push_back(v);
        }

        // Attribute colour c to vertex v.
        coloured[v] = c;

        // Move v from the set of uncoloured to that of coloured vertices.
        uncoloured_vertices.erase(std::remove(uncoloured_vertices.begin(), uncoloured_vertices.end(), v), uncoloured_vertices.end());
        coloured_vertices.push_back(v);

        // WARNING! The following works under the assumption that for each coloured partition, there is at any point
        // only one vertex of the partition that is coloured. That is, the selective colouring is as tight as possible.

        // Move p from the set of uncoloured to that of coloured partitions.
        uncoloured_partitions.erase(std::remove(uncoloured_partitions.begin(), uncoloured_partitions.end(), partition_for[v]), uncoloured_partitions.end());
        coloured_partitions.push_back(partition_for[v]);

        assert(is_valid());
    }

    uint32_t ALNSColouring::score() const {
        assert(is_valid());

        // The score of the colouring is just the number of used colours.
        return n_colours;
    }

    bool ALNSColouring::is_valid() const {
        const auto& graph = g.get();
        for(auto c = 0u; c < colours.size(); ++c) {
            const auto& colour = colours[c];
            for(auto i = 0u; i < colour.size(); ++i) {
                for(auto j = i + 1; j < colour.size(); ++j) {
                    if(graph.connected(colour[i], colour[j])) {
                        std::cout << colour[i] << " and " << colour[j] << " are together in colour " << c << " but they are linked by an edge!" << std::endl;
                        return false;
                    }
                }
            }
        }
        return true;
    }
}
