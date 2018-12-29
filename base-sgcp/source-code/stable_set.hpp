#ifndef _STABLE_SET_HPP
#define _STABLE_SET_HPP

#include "graph.hpp"

#include <functional>

#include <vector>
#include <set>
#include <map>
#include <boost/dynamic_bitset.hpp>

namespace sgcp {
    class StableSet;
    using StableSetCollection = std::vector<StableSet>;

    class StableSet {
        std::reference_wrapper<const Graph> g;
        boost::dynamic_bitset<> intersects_partition;
        VertexIdSet s;

    public:

        // Dummy stable set that covers all vertices.
        // It is always considered valid.
        bool dummy;

    private:

        // Creates a bitset of which partitions are intersected by the set.
        void create_bitset();

        // Tells wether a stable set intersects a partition.
        bool intersects(const std::unordered_set<uint32_t>& p) const;

        // Gives the vertex id of any vertex of the stable set that is in the given partition (by partition or partition id).
        uint32_t any_common_vertex(const std::unordered_set<uint32_t>& p) const;
        uint32_t any_common_vertex(uint32_t k) const;

    public:

        // Creates a dummy stable set.
        StableSet(const Graph& g);

        // Creates a (valid) stable set out of a vertex set.
        StableSet(const Graph& g, VertexIdSet s) : g{g}, intersects_partition{g.n_partitions}, s{s}, dummy{false} { assert(is_valid(true)); create_bitset(); }

        // Creates a (valid) stable set out of a list of vector ids.
        StableSet(const Graph& g, std::vector<uint32_t> sv) : g{g}, intersects_partition{g.n_partitions}, s{sv.begin(), sv.end()}, dummy{false} { assert(is_valid(true)); create_bitset(); }

        // Returns the underlying set of vertex ids.
        const VertexIdSet& get_set() const { return s; }

        // Tells the size of the underlying vertex id set.
        uint32_t size() const { return s.size(); }

        // Adds a vertex to the set and recomputes the bitset.
        void add_vertex(uint32_t id) { s.insert(id); create_bitset(); }

        // Remove a vertex to the set and recomputes the bitset.
        void remove_vertex(uint32_t id) { s.erase(id); create_bitset(); }

        // Tells wether a stable set intersects a partition (by partition id).
        bool intersects(uint32_t k) const;

        // Checks that s actually defines a stable set of the graph.
        bool is_valid(bool print_details) const;

        // Checks if the stable set has a vertex with a specific id.
        bool has_vertex(uint32_t id) const { return s.find(id) != s.cend(); }

        // Calculates the reduced cost of the stable set, given the duals.
        float reduced_cost(const std::vector<float>& duals) const;

        // Useful to use StableSet as key in sorted containers.
        friend bool operator<(const StableSet& lhs, const StableSet& rhs);

        // Useful to prune duplicate columns
        friend bool operator==(const StableSet& lhs, const StableSet& rhs);

        // Prints out the stable set.
        friend std::ostream& operator<<(std::ostream& out, const StableSet& s);

        // Prints a stable set collection. (Needs friend access)
        friend std::ostream& operator<<(std::ostream& out, const StableSetCollection& s);
    };
}

#endif
