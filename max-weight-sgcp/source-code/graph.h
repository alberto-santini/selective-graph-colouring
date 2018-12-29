//
// Created by alberto on 08/05/18.
//

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <cstddef>
#include <iostream>
#include <string>

#ifndef SGCP_VIA_CLIQUES_GRAPH_H
#define SGCP_VIA_CLIQUES_GRAPH_H

namespace sgcp_cliques {
    // Struct used to keep track of the clusters in a clusterd graph.
    struct ClusteredGraphProperties {
        std::size_t num_clusters;
        std::vector<std::vector<std::size_t>> clusters;
    };

    // Type to represent the underlying graph for the SGCP.
    using ClusteredGraph = boost::adjacency_list<
            boost::vecS, boost::vecS, boost::undirectedS,
            std::size_t, // Vertex property: cluster number for the vertex.
            boost::no_property, // No edge property.
            ClusteredGraphProperties // Graph property: number of clusters.
    >;

    // Type to represent the line graph of the original graph.
    // Each vertex of the line graph represents an edge on the original graph.
    // An edge of the line graph between vertices {i,j} and {k,l} means that
    // the two edges have one endpoint in a common cluster in the original graph.
    // This is then extended by the sandwich line graph (which uses the same
    // graph type LineGraph) in which is also required that the two edges
    // do not form a simplicial pair in a given acyclic orientation of the
    // original graph.
    using LineGraph = boost::adjacency_list<
            boost::vecS, boost::vecS, boost::undirectedS,
            std::pair<std::size_t, std::size_t> // Vertex property: the original edge's endpoints.
    >;

    // Type to represent a directed acyclic orientation of the original graph.
    using DirectedGraph = boost::adjacency_list<
            boost::vecS, boost::vecS, boost::directedS,
            std::size_t, // Vertex property: cluster number for the vertex.
            boost::no_property, // No edge property.
            ClusteredGraphProperties // Graph property: number of clusters.
    >;

    // Return the number of partitions, which is stored as a graph property.
    inline std::size_t number_of_partitions(const ClusteredGraph& cgraph) { return cgraph[boost::graph_bundle].num_clusters; }

    // Reads a clustered graph from file.
    // First line: the number N of vertices;
    // Second line: the number M of edges;
    // Third line: the number P of clusters;
    // Next M lines: one edge per line, vertices are separated by one whitespace and numbered from 0 to N-1;
    // Nest P lines: one cluster per line, same as above.
    ClusteredGraph read_clustered_graph(std::string graph_file);

    // Produces an acyclic orientation of the original graph.
    DirectedGraph directed_acyclic(const ClusteredGraph& cgraph);

    // Creates the line graph from the original graph.
    // See the description above the definition of LineGraph.
    LineGraph line_graph(const ClusteredGraph& cgraph);

    // Creates the sandwich line graph from the original graph.
    // See the description above the definition of LineGraph.
    LineGraph sandwich_line_graph(const ClusteredGraph& cgraph);

    // Creates the complementary of the sandwich line graph.
    // This is done to solve a max-clique problem instead of a
    // max-stable-set one.
    LineGraph complementary_sandwich_line_graph(const ClusteredGraph& cgraph);
}

inline std::ostream& operator<<(std::ostream& out, const sgcp_cliques::ClusteredGraph& g) {
    out << boost::num_vertices(g) << "," << boost::num_edges(g) << "," << g[boost::graph_bundle].num_clusters;
    return out;
}

#endif //SGCP_VIA_CLIQUES_GRAPH_H
