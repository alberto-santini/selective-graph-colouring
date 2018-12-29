#ifndef _ALNS_COLOURING_HPP
#define _ALNS_COLOURING_HPP

#include "../../graph.hpp"
#include <vector>
#include <random>
#include <branch-and-price/column_pool.hpp>

namespace sgcp {

    // This class represents a complete or incomplete selective colouring of a graph.
    // The colouring is incomplete after being destroyed by a DestroyMove.
    // The colouring is complete after being repaired by a RepairMove.
    // In the beginning, the colouring must be complete and be produced by
    // come constructive heuristic.
    struct ALNSColouring {
        // Reference to the graph
        std::reference_wrapper<const Graph> g;

        // Number of colours currently being used. If the colouring is complete,
        // this is basically the score of the current colouring.
        uint32_t n_colours;

        // THis attributes to each colour in [0, n_colours - 1] an id. The main
        // property of the id is that it's associated to a certain colour rather
        // than to the position of that colour in vector ``colours''. This is
        // better explained with an example. Let's start with three colours:
        // colours[0] = { 1, 2, 3 }; id[0] = 0;
        // colours[1] = { 4 };       id[1] = 1;
        // colours[2] = { 5, 6 };    id[2] = 2;
        // If I decide to uncolour vertex 4, colour 1 will be destroyed and
        // colour 2 will be shifted back in position 1, but its id will remain 2:
        // colours[0] = { 1, 2, 3 }; id[0] = 0;
        // colours[1] = { 5, 6 };    id[2] = 1;
        // This is done so that classes that store the colour id for many
        // iterations can still have a valid reference to the colour, even when
        // that colour is shifted. An example is a tabu move which says that
        // vertex v cannot be coloured with a colour with a certain id.
        std::vector<uint32_t> id;

        // colours[i] contains the list of vertices coloured with colour i.
        // The colours used are always contiguous from 0 to n_colours - 1.
        std::vector<std::vector<uint32_t>> colours;

        // coloured[v] gives the colour with which vertex v is coloured.
        // If v is uncoloured, the key v will not be in the map.
        std::unordered_map<uint32_t, uint32_t> coloured;

        // List of all coloured vertices. This is basically the union
        // of colours[i] for all i.
        std::vector<uint32_t> coloured_vertices;

        // Lis of all uncoloured vertices. This is the list of all
        // vertices, minus coloured_vertices. When this is empty,
        // then the colouring is complete; when it is non-empty, the
        // colouring is incomplete.
        std::vector<uint32_t> uncoloured_vertices;

        // List of all coloured partitions. This is basically the
        // projection of coloured_vertices via the functor that assigns
        // to each vertex v its partition p(v).
        std::vector<uint32_t> coloured_partitions;

        // Lis of all uncoloured partitions. This is the list of all
        // paritions, minus coloured_partitions.
        std::vector<uint32_t> uncoloured_partitions;

        // This (redundant) map is a convenient way to know to which
        // partition each vertex belongs. It could be replaced by
        // only using Graph methods, but that would be more expensive.
        std::unordered_map<uint32_t, uint32_t> partition_for;

        // Creates an empty colouring for graph g.
        ALNSColouring(const Graph& g);

        // Creates a colouring for graph g, from a solution.
        ALNSColouring(const Graph& g, const ColumnPool& cp);

        // Gives a ColumnPool representing the colouring.
        ColumnPool to_column_pool() const;

        // Removes coloured vertex v from its colour and puts it in the
        // uncoloured vertices pool. It update all other member variables
        // accordingly.
        void uncolour_vertex(uint32_t v);

        // Removes coloured vertex v from its colour and puts it in the
        // uncoloured vertices pool. It update all other member variables
        // accordingly. Here v is the (only) coloured vertex which belongs
        // to partition p.
        void uncolour_partition(uint32_t p);

        // Removes uncoloured vertex v from the uncoloured pool and places
        // it in colour c. If c == n_colours, it creates a new colour.
        void colour_vertex(uint32_t v, uint32_t c);

        // Return the score of a colouring, which is the number of used colours.
        // This only makes sense for complete colourings.
        uint32_t score() const;

        // Makes sure that no two vertices in the same colour share an edge.
        bool is_valid() const;
    };

}

#endif
