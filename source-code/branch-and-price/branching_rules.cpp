#include "branching_rules.hpp"

#include <boost/graph/copy.hpp>

namespace sgcp {
    std::shared_ptr<const Graph> VerticesRemoveRule::apply() const {
        assert(std::all_of(vertices_id.begin(), vertices_id.end(), [&] (auto i) { return i < g->n_vertices; }));

        BoostGraph new_bg;
        Partition new_p;
        std::map<uint32_t, uint32_t> id_shift;
        auto new_v_idx = 0u;

        for(auto it = vertices(g->g); it.first != it.second; ++it.first) {
            Vertex v = *it.first;
            uint32_t v_id = g->g[v].id;

            if(std::find(vertices_id.begin(), vertices_id.end(), v_id) == vertices_id.end()) {
                Vertex new_v = add_vertex(new_bg);
                new_bg[new_v] = VertexInfo{new_v_idx++, g->g[v].represented_vertices};
                id_shift[v_id] = new_v_idx - 1;
            }
        }

        for(auto it = edges(g->g); it.first != it.second; ++it.first) {
            Edge e = *it.first;
            Vertex v_from = source(e, g->g);
            Vertex v_to = target(e, g->g);
            uint32_t from_id = g->g[v_from].id;
            uint32_t to_id = g->g[v_to].id;

            if( std::find(vertices_id.begin(), vertices_id.end(), from_id) == vertices_id.end() &&
                std::find(vertices_id.begin(), vertices_id.end(), to_id) == vertices_id.end())
            {
                // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
                // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
                auto new_v_from = boost::make_optional(false, Vertex{});
                auto new_v_to = boost::make_optional(false, Vertex{});

                for(auto vit = vertices(new_bg); vit.first != vit.second; ++vit.first) {
                    if(!new_v_from && new_bg[*vit.first].id == id_shift.at(from_id)) { new_v_from = *vit.first; if(new_v_to) { break; } }
                    if(!new_v_to && new_bg[*vit.first].id == id_shift.at(to_id)) { new_v_to = *vit.first; if(new_v_from) { break; } }
                }
                assert(new_v_from && new_v_to);

                add_edge(*new_v_from, *new_v_to, new_bg);
            }
        }

        for(auto k = 0u; k < g->n_partitions; k++) {
            auto new_set = std::unordered_set<uint32_t>();
            for(auto id : g->p[k]) {
                if(std::find(vertices_id.begin(), vertices_id.end(), id) == vertices_id.end()) {
                    auto vit = vertices(new_bg);
                    auto new_v = std::find_if(vit.first, vit.second, [&] (auto v) { return new_bg[v].id == id_shift.at(id); });

                    assert(new_v != vit.second);

                    new_set.insert(*new_v);
                }
            }
            new_p.push_back(new_set);
        }

        return std::make_shared<const Graph>(new_bg, new_p, g->params);
    }

    std::shared_ptr<const Graph> VerticesLinkRule::apply() const {
        assert(i1 < g->n_vertices);
        assert(i2 < g->n_vertices);

        BoostGraph new_bg;
        Partition new_p;

        copy_graph(g->g, new_bg);

        // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
        auto new_v1 = boost::make_optional(false, Vertex{});
        auto new_v2 = boost::make_optional(false, Vertex{});

        for(auto it = vertices(new_bg); it.first != it.second; ++it.first) {
            if(!new_v1 && new_bg[*it.first].id == i1) { new_v1 = *it.first; if(new_v2) { break; } }
            if(!new_v2 && new_bg[*it.first].id == i2) { new_v2 = *it.first; if(new_v1) { break; } }
        }
        assert(new_v1 && new_v2);
        assert(!edge(*new_v1, *new_v2, new_bg).second);

        add_edge(*new_v1, *new_v2, new_bg);

        for(auto k = 0u; k < g->n_partitions; k++) {
            auto new_set = std::unordered_set<uint32_t>();
            for(auto id : g->p[k]) {
                auto vit = vertices(new_bg);
                auto new_v = std::find_if(vit.first, vit.second, [&] (auto v) { return new_bg[v].id == id; });

                assert(new_v != vit.second);

                new_set.insert(*new_v);
            }

            assert(new_set.size() == g->p[k].size());
            new_p.push_back(new_set);
        }

        return std::make_shared<const Graph>(new_bg, new_p, g->params);
    }

    std::shared_ptr<const Graph> VerticesMergeRule::apply() const {
        assert(i1 < g->n_vertices);
        assert(i2 < g->n_vertices);

        BoostGraph new_bg;
        Partition new_p;
        std::map<uint32_t, uint32_t> id_shift;
        auto new_v_idx = 0u;

        auto v1 = g->vertex_by_id(i1);
        auto v2 = g->vertex_by_id(i2);
        assert(v1 && v2);
        assert(!edge(*v1, *v2, g->g).second);

        for(auto it = vertices(g->g); it.first != it.second; ++it.first) {
            Vertex v = *it.first;
            if(v != *v1 && v != *v2) {
                Vertex new_v = add_vertex(new_bg);
                new_bg[new_v] = VertexInfo{new_v_idx++, g->g[v].represented_vertices};
                id_shift[g->g[v].id] = new_v_idx - 1;
            }
        }

        Vertex merged_v = add_vertex(new_bg);
        uint32_t merged_id = new_v_idx;
        std::vector<uint32_t> merged_rep_v = g->g[*v1].represented_vertices;
        merged_rep_v.insert(merged_rep_v.end(), g->g[*v2].represented_vertices.begin(), g->g[*v2].represented_vertices.end());

        new_bg[merged_v] = VertexInfo{merged_id, merged_rep_v};

        for(auto it = edges(g->g); it.first != it.second; ++it.first) {
            Edge e = *it.first;
            Vertex v_from = source(e, g->g);
            Vertex v_to = target(e, g->g);
            uint32_t from_id = g->g[v_from].id;
            uint32_t to_id = g->g[v_to].id;

            // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
            auto new_v_from = boost::make_optional(false, Vertex{});
            auto new_v_to = boost::make_optional(false, Vertex{});

            if(from_id == i1 || from_id == i2) { new_v_from = merged_v; }
            if(to_id == i1 || to_id == i2) { new_v_to = merged_v; }

            for(auto vit = vertices(new_bg); vit.first != vit.second; ++vit.first) {
                if(!new_v_from && new_bg[*vit.first].id == id_shift.at(from_id)) { new_v_from = *vit.first; if(new_v_to) { break; } }
                if(!new_v_to && new_bg[*vit.first].id == id_shift.at(to_id)) { new_v_to = *vit.first; if(new_v_from) { break; } }
            }
            assert(new_v_from && new_v_to);

            if(!edge(*new_v_from, *new_v_to, new_bg).second) { add_edge(*new_v_from, *new_v_to, new_bg); }
        }

        for(auto k = 0u; k < g->n_partitions; k++) {
            auto new_set = std::unordered_set<uint32_t>();
            for(auto id : g->p[k]) {
                // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
                // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
                boost::optional<Vertex> new_v = boost::make_optional(false, Vertex{});

                if(id == i1 || id == i2) { new_v = merged_v; }
                else {
                    for(auto vit = vertices(new_bg); vit.first != vit.second; ++vit.first) {
                        if(new_bg[*vit.first].id == id_shift.at(id)) { new_v = *vit.first; }
                    }
                }
                assert(new_v);

                new_set.insert(g->g[*new_v].id);
            }
            new_p.push_back(new_set);
        }

        return std::make_shared<const Graph>(new_bg, new_p, g->params);
    }

    bool BranchingRule::vertex_in_set(uint32_t id, const VertexIdSet& s) const {
        return std::find_if(
            s.begin(),
            s.end(),
            [&] (uint32_t vs_id) -> bool {
                auto v = g->vertex_by_id(id);
                assert(v);
                return g->g[*v].represents(vs_id);
            }
        ) != s.end();
    }

    bool VerticesRemoveRule::is_compatible(const StableSet& s) const {
        // A stable set S is compatible with this rule if none of the vertices removed by
        // the rule is present in set S. The vertices involved by the rule is the union of
        // all the original vertices corresponding to each vertex in vertices_id.

        // Dummy column always compatible
        if(s.dummy) { return true; }

        return std::none_of(
            vertices_id.begin(),
            vertices_id.end(),
            [&] (uint32_t id) -> bool { return vertex_in_set(id, s.get_set()); }
        );
    }

    bool VerticesLinkRule::is_compatible(const StableSet& s) const {
        // A stable set S is compatible with this rule if it contains at most one among
        // i1 and i2 (the two linked vertices).

        // Dummy column always compatible
        if(s.dummy) { return true; }

        return !(vertex_in_set(i1, s.get_set()) && vertex_in_set(i2, s.get_set()));
    }

    bool VerticesMergeRule::is_compatible(const StableSet& s) const {
        // A stable set S is compatible with this rule if it does not contain exactly
        // one of the two merged vertices i1 and i2.

        // Dummy column always compatible
        if(s.dummy) { return true; }

        return (vertex_in_set(i1, s.get_set()) == vertex_in_set(i2, s.get_set()));
    }
}