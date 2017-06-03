#ifndef _BRANCHING_HELPER_HPP
#define _BRANCHING_HELPER_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"
#include "bb_solution.hpp"
#include "column_pool.hpp"

#include <set>
#include <map>
#include <vector>

#include <boost/optional.hpp>

namespace sgcp {
    class BranchingHelper {
        const Graph& g;
        const BBSolution& sol;
        const ColumnPool& column_pool;

        // Returns how many vertices of a given partition are coloured by a given set of vertices.
        // The ids in the set of vertices refer to the original ids.
        uint32_t how_many_intersections(uint32_t k, const VertexIdSet& s) const;

        // Returns any vertex in the intersection between the k-th partition and the given set of vertices.
        // The ids in the set of vertices refer to the original ids.
        boost::optional<uint32_t> any_vertex_in_intersection(uint32_t k, const VertexIdSet& s) const;

        // Tells if a given set contains a certain vertex (by id).
        // The vertex id refers to the current ids, but the set ids refer to the original ones.
        bool is_vertex_in_set(uint32_t id, const VertexIdSet& s) const;

        // Tells, for each vertex, if there is (at least) one column in the LP solution that covers it.
        std::map<uint32_t, bool> mark_coloured_vertices_by_lp_solution() const;

        // Counts how many vertices have been coloured, in each partition, according to a provided LP solution.
        std::map<uint32_t, uint32_t> count_coloured_vertices_by_lp_solution_in_each_partition() const;
        
        // Gets the id of a partition that has more than one coloured vertex, according to a provided LP solution, if any.
        // Check the implementation to see how ties are broken.
        boost::optional<uint32_t> partition_with_more_than_one_coloured_vertex() const;

        // Gets the id of a vertex, if any, that is both in the most fractional column that covers a partition -- and in the partition itself.
        boost::optional<uint32_t> vertex_in_most_fractional_column_that_covers_partition(uint32_t part_k) const;

        static constexpr float eps = 1e-6;

    public:
        BranchingHelper(const Graph& g, const BBSolution& sol, const ColumnPool& column_pool) :
            g{g}, sol{sol}, column_pool{column_pool} {}

        // Returns any vertex of the graph contained in the given set, if any (remember
        // some vertices are removed during the branching process). The ids in the set
        // refer to the original ids.
        boost::optional<uint32_t> any_vertex_in_set(const VertexIdSet& s) const;

        // Finds any vertex that is part of exactly one of the two provided columns.
        // The ids in the sets of vertices (in the columns) refer to the original ids.
        boost::optional<uint32_t> any_vertex_covered_by_exactly_one_column(uint32_t c1, uint32_t c2) const;
        
        // Returns a pair of partition id and vertex id. The vertex is the most fractional one, out of all
        // vertices belonging to partitions coloured by more than one colour.
        boost::optional<std::pair<uint32_t, uint32_t>> most_fractional_vertex_in_partition_with_more_than_one_coloured_vertex() const;

        // Returns the most fractional column in the LP solution.
        uint32_t most_fractional_column() const;

        // Given the id of a vertex and that of a column that covers the vertex, find another column, if any, that covers the same vertex.
        boost::optional<uint32_t> another_column_covering_vertex(uint32_t column1_id, uint32_t vertex_id) const;
    };
}

#endif