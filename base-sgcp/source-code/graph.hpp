#ifndef _GRAPH_HPP
#define _GRAPH_HPP

#include "params.hpp"

#include <boost/optional.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>

#include <iostream>
#include <string>

namespace sgcp {
    struct VertexInfo {
        // Vertex id, with the only requirement that it is unique.
        uint32_t id;

        // Keeps track of merged or reindexed vertices. The value in here
        // always refer to the original indexing of the vertices.
        std::vector<uint32_t> represented_vertices;

        bool represents(uint32_t id) const { return std::find(represented_vertices.begin(), represented_vertices.end(), id) != represented_vertices.end(); }
    };

    std::ostream& operator<<(std::ostream& out, const VertexInfo& v);

    using BoostGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, VertexInfo>;
    using Vertex = boost::graph_traits<BoostGraph>::vertex_descriptor;
    using Edge = boost::graph_traits<BoostGraph>::edge_descriptor;
    using Partition = std::vector<std::unordered_set<uint32_t>>;
    using WeightMap = std::map<uint32_t, float>;
    using VertexIdSet = std::unordered_set<uint32_t>;

    class Graph {
        void renumber_vertices();
        void do_preprocessing();
        void erase_vertex(Vertex v);
        void remove_partitions(std::vector<uint32_t> removable);
        void make_partition_cliques();
        bool is_cover_or_partition_valid(bool must_also_be_partition) const;

    public:
        uint32_t n_vertices;
        uint32_t n_edges;
        uint32_t n_partitions;

        BoostGraph g;
        Partition p;
        Params params;

        std::string data_filename;

        Graph(std::string filename, std::string params_filename);
        Graph(BoostGraph g, Partition p, Params params);

        // Checks that the partitions cover the whole graph and don't overlap.
        bool is_partition_valid() const;

        // Checks that the partitions cover the whole graph (they may possibly overlap).
        bool is_cover_valid() const;

        // Tells whether or not there is an edge between the two vertices.
        // The first version uses current ids, the second original ids.
        bool connected(uint32_t i, uint32_t j) const;
        bool connected_by_original_id(uint32_t i, uint32_t j) const;

        // Returns a vertex by its id, if it exists.
        boost::optional<Vertex> vertex_by_id(uint32_t id) const;

        // Returns a vertex by its id, if it exists. The vertex id refers to the original ids.
        boost::optional<Vertex> vertex_by_original_id(uint32_t id) const;

        // Check wether the provided stable set is compatible with the current graph.
        // The ids refer to the original ids. Causes of incompatibility stem from the
        // modification (linking, merging, removal) to the graph.
        bool is_compatible_as_stable_set(const VertexIdSet& s) const;

        // Returns the index of the partition for a vertex with a certain id.
        uint32_t partition_for(uint32_t i) const;

        // Returns the anti-neighbourhood of i, i.e. the ids of all the vertices that are
        // not linked to i by an edges. The first versions use current ids, the second original ids.
        VertexIdSet anti_neighbourhood_of(uint32_t i, bool including_itself = false) const;
        VertexIdSet anti_neighbourhood_including_itself_of(uint32_t i) const { return anti_neighbourhood_of(i, true); }
        VertexIdSet original_id_anti_neighbourhood_of(uint32_t i, bool including_itself = false) const;
        VertexIdSet original_id_anti_neighbourhood_including_itself_of(uint32_t i) const { return original_id_anti_neighbourhood_of(i, true); }
    };

    std::ostream& operator<<(std::ostream& out, const Graph& g);
}

#endif