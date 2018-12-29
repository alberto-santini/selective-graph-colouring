#ifndef _TABU_SEARCH_HPP
#define _TABU_SEARCH_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"

#include <random>
#include <vector>
#include <set>
#include <map>

namespace sgcp {
    class TabuSearchSolver {
        struct PartitionsColourStatus {
            std::set<uint32_t> coloured;
            std::set<uint32_t> uncoloured;
        };
    
        struct TabuElement {
            uint32_t colour;
            uint32_t vertex_id;
            
            bool operator<(const TabuElement& e) const { return e.colour < colour && e.vertex_id < vertex_id; }
        };
    
        struct InsertionResult {
            std::vector<uint32_t> removed_vertices;
            std::set<uint32_t> uncoloured_partitions;
            uint32_t inserted_vertex;
            uint32_t coloured_partition;
            uint32_t colour;
            uint32_t score;
        };
    
        const Graph& g;
    
        // Best solution found so far.
        std::vector<StableSetCollection> solutions;
    
        // Looking for a colouring with target_colours_n colours.
        uint32_t target_colours_n;
    
        // The colouring we wish to produce. This vector will
        // have exactly target_colours_n sets.
        StableSetCollection colouring_stable_sets;
    
        // The "last" set, in which we put all the vertices we do
        // not manage (or do not need) to colour.
        VertexIdSet uncoloured_set;
    
        // Tells which partitions are currently coloured
        PartitionsColourStatus partitions_colour_status;
    
        // Current iteration number
        uint32_t iteration_n;
    
        // Tabu list, where each element has an associated number,
        // which represent the iteration at which it will be removed
        // from the list.
        std::map<TabuElement, uint32_t> tabu_list;
        
        // Initialises the solver
        void initialise(StableSetCollection initial_solution);
    
        // Gets the id of any partition that is not yet coloured.
        uint32_t random_uncoloured_partition(std::mt19937& mt) const;
    
        // Gets the id of any vertex in a partition. Requires that the
        // partition is uncoloured.
        uint32_t random_uncoloured_vertex(uint32_t partition, std::mt19937& mt) const;
    
        // Tells wether there is a colour bucket where vertex can be put.
        // The answer will be no, if all pairs (colour bucket, vertex) are
        // currently in the tabu list.
        bool is_colourable(uint32_t vertex) const;
        
        // Gives the degree of the vertex, but only considering edges that
        // link it to vertices that are not in its same partition.
        uint32_t external_degree(Vertex vertex, uint32_t partition) const;
    
        // Tells what would happen if a vertex were put in a certain
        // colour bucket. It assumes that the operation is legit, i.e.
        // the vertex is currently uncoloured, and its partition is
        // uncoloured as well.
        InsertionResult simulate_insertion(uint32_t vertex, uint32_t partition, uint32_t colour) const;
    
        // Actually perform the insertion.
        // It also puts the corresponding move in the rabu list, for
        // the number of iterations specified in ``tenure''.
        void insert(const InsertionResult& r, uint32_t tenure);
    
        // Tells wether all partitions have been coloured.
        bool all_partitions_coloured() const;
        
        // Return the index of the smallest set.
        uint32_t smallest_set(const StableSetCollection& s);
        
        // Removes expired items from the tabu list.
        void update_tabu_list();

        // Solves one ``macro-iteration'', attempting to decrease the number of colours by one.
        std::vector<StableSetCollection> solve_iter(const StableSetCollection& initial);

    public:
        TabuSearchSolver(const Graph& g) : g{g} {}

        // Keeps applying tabu search to successive solutions,
        // until it is no more possible to decrease the number
        // of colours.
        StableSetCollection solve(boost::optional<StableSetCollection> initial_solution = boost::none, float* elapsed_time = nullptr);
    };
}

#endif