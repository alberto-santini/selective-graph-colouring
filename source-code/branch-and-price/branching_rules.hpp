#ifndef _BRANCHING_RULES_HPP
#define _BRANCHING_RULES_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"

#include <set>
#include <memory>
#include <vector>

namespace sgcp {
    class BranchingRule {
    protected:
        std::shared_ptr<const Graph> g;

        // Tells if the vertex (given by its current id) is contained
        // in a set of vertices (given by their original ids).
        bool vertex_in_set(uint32_t id, const VertexIdSet& s) const;

    public:
        BranchingRule(std::shared_ptr<const Graph> g) : g{g} {}

        // Apply the rule to a graph, obtaining a new graph
        virtual std::shared_ptr<const Graph> apply() const = 0;

        // Checks if the given stable set is compatible with the branching rule
        // (and therefore can stay in the column pool of the corresponding B&B node).
        virtual bool is_compatible(const StableSet& s) const = 0;
    };

    // Does noting
    class EmptyRule : public BranchingRule {
    public:
        EmptyRule(std::shared_ptr<const Graph> g) : BranchingRule{g} {}
        std::shared_ptr<const Graph> apply() const { return g; }
        bool is_compatible(const StableSet& s) const { return true; }
    };

    // Removes certain vertices from the graph
    class VerticesRemoveRule : public BranchingRule {
        std::vector<uint32_t> vertices_id;

    public:
        VerticesRemoveRule(std::shared_ptr<const Graph> g, std::vector<uint32_t> vertices_id) : BranchingRule(g), vertices_id{vertices_id} {
            // Check that all elements of vertices_id are unique.
            assert(std::set<uint32_t>(vertices_id.begin(), vertices_id.end()).size() == vertices_id.size());
        }

        std::shared_ptr<const Graph> apply() const;
        bool is_compatible(const StableSet& s) const;
    };

    // Connects two vertices in the graph
    class VerticesLinkRule : public BranchingRule {
        uint32_t i1;
        uint32_t i2;

    public:
        VerticesLinkRule(std::shared_ptr<const Graph> g, uint32_t i1, uint32_t i2) : BranchingRule(g), i1{i1}, i2{i2} { assert(i1 != i2); }

        std::shared_ptr<const Graph> apply() const;
        bool is_compatible(const StableSet& s) const;
    };

    class VerticesMergeRule : public BranchingRule {
        uint32_t i1;
        uint32_t i2;

    public:
        VerticesMergeRule(std::shared_ptr<const Graph> g, uint32_t i1, uint32_t i2) : BranchingRule(g), i1{i1}, i2{i2} { assert(i1 != i2); }

        std::shared_ptr<const Graph> apply() const;
        bool is_compatible(const StableSet& s) const;
    };
}

#endif