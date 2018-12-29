#include "graph.hpp"

#include "utils/console_colour.hpp"
#include "utils/dbg_output.hpp"

#include <iostream>
#include <fstream>
#include <stdexcept>

namespace sgcp {
    std::ostream& operator<<(std::ostream& out, const VertexInfo& v) {
        using namespace Console;

        out << v.id << " representing ";

        auto last_it = std::prev(v.represented_vertices.end());
        for(auto it = v.represented_vertices.begin(); it != last_it; ++it) {
            out << *it << ", ";
        }
        out << *last_it;

        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Graph& g) {
        using namespace Console;

        out << colour_magenta("Vertices") << std::endl;
        for(auto it = vertices(g.g); it.first != it.second; ++it.first) {
            out << "\t" << g.g[*it.first] << std::endl;
        }
        for(auto k = 0u; k < g.n_partitions; k++) {
            out << colour_magenta("Partition ") << colour_magenta(k) << std::endl;
            for(auto i : g.p[k]) {
                auto v = g.vertex_by_id(i);
                assert(v);
                out << "\t" << g.g[*v] << std::endl;
            }
        }
        out << colour_magenta("Edges") << std::endl;
        for(auto it = edges(g.g); it.first != it.second; ++it.first) {
            out << "\t" << g.g[source(*it.first, g.g)] << " => " << g.g[target(*it.first, g.g)] << std::endl;
        }

        return out;
    }

    Graph::Graph(BoostGraph g, Partition p, Params params) : g{g}, p{p}, params{params} {
        n_vertices = num_vertices(g);
        n_edges = num_edges(g);
        n_partitions = p.size();
        data_filename = "";
    }

    Graph::Graph(std::string filename, std::string params_filename) : params{params_filename}, data_filename{filename} {
        DEBUG_ONLY(using namespace Console;)
        std::ifstream gfile(filename, std::ios_base::in);

        DEBUG_ONLY(std::cout << colour_magenta("Reading graph in ") << colour_magenta(filename) << std::endl;)

        gfile >> n_vertices;
        gfile >> n_edges;
        gfile >> n_partitions;

        assert(n_vertices > 0u);
        assert(n_partitions > 0u);

        DEBUG_ONLY(std::cout << "\t" << colour_magenta(n_vertices) << " vertices" << std::endl;)
        DEBUG_ONLY(std::cout << "\t" << colour_magenta(n_edges) << " edges" << std::endl;)
        DEBUG_ONLY(std::cout << "\t" << colour_magenta(n_partitions) << " partitions" << std::endl;)

        for(uint32_t i = 0u; i < n_vertices; i++) {
            auto v = add_vertex(g);
            g[v] = VertexInfo{i, {i}};
        }

        for(uint32_t n_line = 0u; n_line < n_edges; n_line++) {
            uint32_t from, to;

            gfile >> from >> to;
            assert(from < n_vertices);
            assert(to < n_vertices);

            auto from_v = vertex_by_id(from);
            auto to_v = vertex_by_id(to);
            assert(from_v);
            assert(to_v);

            add_edge(*from_v, *to_v, g);
        }

        std::stringstream ss("");
        std::string line;

        gfile >> std::ws;

        for(uint32_t n_line = 0u; n_line < n_partitions; n_line++) {
            std::getline(gfile, line);
            ss.str(line);

            p.push_back(std::unordered_set<uint32_t>());

            uint32_t element;
            while(ss >> element) {
                assert(element < n_vertices);
                p.back().insert(element);
            }

            ss.str(""); ss.clear();
        }

        assert(num_vertices(g) == n_vertices);
        assert(num_edges(g) == n_edges);
        assert(p.size() == n_partitions);

        make_partition_cliques();
        do_preprocessing();
        renumber_vertices();

        // Update the data, after preprocessing
        n_vertices = num_vertices(g);
        n_edges = num_edges(g);
        n_partitions = p.size();
    }

    void Graph::renumber_vertices() {
        std::unordered_map<uint32_t, uint32_t> rn;
        uint32_t n = 0u;

        for(auto it = vertices(g); it.first != it.second; ++it.first) {
            auto v = *it.first;
            rn[g[v].id] = n;
            g[v].id = n;
            g[v].represented_vertices = {n};
            n++;
        }

        Partition new_p;
        new_p.reserve(n_partitions);
        
        for(const auto& s : p) {
            std::unordered_set<uint32_t> new_set;
            for(const auto& id : s) {
                new_set.insert(rn[id]);
            }
            new_p.push_back(new_set);
        }
        
        p = new_p;
    }

    void Graph::do_preprocessing() {
        DEBUG_ONLY(using namespace Console;)

        // 1) If in a partition P there is a vertex v, which is only
        // linked to other vertices of P, then I can remove the whole
        // partition, because it is always possible to colour v, with
        // any colour we want.
        std::vector<uint32_t> removable;
        for(auto k = 0u; k < p.size(); k++) {
            for(auto v_id : p[k]) {
                auto v = vertex_by_id(v_id);
                assert(v);

                auto it = out_edges(*v, g);
                if(std::all_of(
                    it.first, it.second,
                    [&] (const auto& e) {
                        auto w_id = g[target(e, g)].id;
                        return std::find(p[k].begin(), p[k].end(), w_id) != p[k].end();
                    }
                )) {
                    removable.push_back(k);
                    break;
                }
            }
        }
        remove_partitions(removable);
        DEBUG_ONLY(std::cout << "Preprocessing removed " << removable.size() << " partitions." << std::endl;)

        // 2) If v and w are two vertices in the same partition, and
        // N(v) contains N(w), then I can remove v: in any solution
        // in which I colour v, I can colour w with the same colour.
        std::vector<Vertex> remove_v;
        for(auto k = 0u; k < p.size(); k++) {
            if(p[k].size() < 2u) { continue; }
            for(const auto& i_id : p[k]) {
                auto vi = vertex_by_id(i_id);
                assert(vi);
                for(const auto& j_id : p[k]) {
                    if(j_id == i_id) { continue; }

                    auto vj = vertex_by_id(j_id);
                    assert(vj);

                    auto it_i = out_edges(*vi, g);
                    auto it_j = out_edges(*vj, g);

                    if(std::all_of(
                        it_i.first,
                        it_i.second,
                        [&] (const auto& wi) { return std::find(it_j.first, it_j.second, wi) != it_j.second; }
                    )) {
                        remove_v.push_back(*vj);
                        continue;
                    }

                    if(std::all_of(
                        it_j.first,
                        it_j.second,
                        [&] (const auto& wj) { return std::find(it_i.first, it_i.second, wj) != it_i.second; }
                    )) {
                        remove_v.push_back(*vi);
                        break;
                    }
                }
            }
        }
        for(auto v : remove_v) { erase_vertex(v); }
        DEBUG_ONLY(std::cout << "Preprocessing removed " << remove_v.size() << " additional vertices." << std::endl;)
    }

    void Graph::erase_vertex(Vertex v) {
        auto k = partition_for(g[v].id);
        p[k].erase(g[v].id);
        clear_vertex(v, g);
        remove_vertex(v, g);
    }

    void Graph::remove_partitions(std::vector<uint32_t> removable) {
        for(auto k : removable) {
            while(!p[k].empty()) {
                auto v = vertex_by_id(*p[k].begin());
                assert(v);

                p[k].erase(p[k].begin());
                clear_vertex(*v, g);
                remove_vertex(*v, g);
            }
        }
        p.erase(std::remove_if(p.begin(), p.end(), [] (const auto& s) { return s.empty(); }), p.end());
    }

    void Graph::make_partition_cliques() {
        for(const auto& s : p) {
            for(auto it = s.begin(); it != s.end(); ++it) {
                auto vi = vertex_by_id(*it);
                assert(vi);

                for(auto jt = std::next(it); jt != s.end(); ++jt) {
                    auto vj = vertex_by_id(*jt);
                    assert(vj);

                    if(!edge(*vi, *vj, g).second) { add_edge(*vi, *vj, g); }
                }
            }
        }
    }

    bool Graph::is_cover_or_partition_valid(bool must_also_be_partition) const {
        DEBUG_ONLY(using namespace Console;)

        for(auto vit = vertices(g); vit.first != vit.second; ++vit.first) {
            uint32_t sets_n = 0u;
            uint32_t v_id = g[*vit.first].id;

            for(const auto& s : p) { if(s.find(v_id) != s.end()) { sets_n++; } }

            if(sets_n == 0u || (must_also_be_partition && sets_n > 1u)) {
                DEBUG_ONLY(std::cerr << "Vertex " << g[*vit.first] << " is in " << sets_n << " partitions." << std::endl;)
                return false;
            }
        }
        return true;
    }

    bool Graph::is_cover_valid() const {
        return is_cover_or_partition_valid(false);
    }

    bool Graph::is_partition_valid() const {
        return is_cover_or_partition_valid(true);
    }

    boost::optional<Vertex> Graph::vertex_by_id(uint32_t id) const {
        for(auto it = vertices(g); it.first != it.second; ++it.first) {
            if(g[*it.first].id == id) { return *it.first; }
        }
        return boost::none;
    }

    boost::optional<Vertex> Graph::vertex_by_original_id(uint32_t id) const {
        for(auto it = vertices(g); it.first != it.second; ++it.first) {
            Vertex v = *it.first;
            if(g[v].represents(id)) { return v; }
        }
        return boost::none;
    }

    uint32_t Graph::partition_for(uint32_t i) const {
        for(uint32_t k = 0u; k < n_partitions; k++) {
            if(std::find(p[k].begin(), p[k].end(), i) != p[k].end()) { return k; }
        }

        // If we reach this point, there is a vertex that is in no partitions
        assert(false); return n_vertices;
    }

    bool Graph::connected(uint32_t i, uint32_t j) const {
        auto vi = vertex_by_id(i);
        if(!vi) { return false; }

        auto vj = vertex_by_id(j);
        if(!vj) { return false; }

        return edge(*vi, *vj, g).second;
    }

    bool Graph::connected_by_original_id(uint32_t i, uint32_t j) const {
        auto vi = vertex_by_original_id(i);
        if(!vi) { return false; }

        auto vj = vertex_by_original_id(j);
        if(!vj) { return false; }

        return edge(*vi, *vj, g).second;
    }

    bool Graph::is_compatible_as_stable_set(const VertexIdSet& s) const {
        // 1) It contains any removed vertex.
        for(auto id : s) {
            auto v = vertex_by_original_id(id);
            if(!v) { return false; }
        }

        // 2) If it contains any vertex part of a "fat" vertex, it
        // must contain all other merged vertices part of the same
        // "fat" vertex.
        for(auto it = vertices(g); it.first != it.second; ++it.first) {
            if(g[*it.first].represented_vertices.size() > 1u) {
                uint32_t covered_vertices = 0u;

                for(auto r_id : g[*it.first].represented_vertices) {
                    if(std::find(s.begin(), s.end(), r_id) != s.end()) { covered_vertices++; }
                }

                if(covered_vertices != 0u && covered_vertices != g[*it.first].represented_vertices.size()) { return false; }
            }
        }

        // 3) It does not contain any 2 vertices linked by an edge.
        for(auto it = s.begin(); it != s.end(); ++it) {
            auto vi = vertex_by_original_id(*it);
            assert(vi);
            for(auto jt = std::next(it); jt != s.end(); ++jt) {
                auto vj = vertex_by_original_id(*jt);
                assert(vj);

                if(edge(*vi, *vj, g).second) { return false; }
            }
        }

        return true;
    }

    VertexIdSet Graph::original_id_anti_neighbourhood_of(uint32_t i, bool including_itself) const {
        auto v = vertex_by_original_id(i);
        VertexIdSet n;

        if(!v) { return n; }

        for(auto it = vertices(g); it.first != it.second; ++it.first) {
            if(*it.first == *v && !including_itself) { continue; }
            if(edge(*v, *it.first, g).second) { continue; }
            std::copy(g[*it.first].represented_vertices.begin(), g[*it.first].represented_vertices.end(), std::inserter(n, n.end()));
        }

        return n;
    }

    VertexIdSet Graph::anti_neighbourhood_of(uint32_t i, bool including_itself) const {
        auto v = vertex_by_id(i);
        VertexIdSet n;

        if(!v) { return n; }

        for(auto it = vertices(g); it.first != it.second; ++it.first) {
            if(*it.first == *v && !including_itself) { continue; }
            if(edge(*v, *it.first, g).second) { continue; }
            n.insert(g[*it.first].id);
        }

        return n;
    }
}
