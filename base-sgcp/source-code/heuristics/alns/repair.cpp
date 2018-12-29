#include "repair.hpp"

#include <vector>

namespace sgcp {
    void InsertRandomVertexInRandomColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            std::uniform_int_distribution<uint32_t> dis(0, g.p[p].size() - 1);
            auto it = g.p[p].begin();
            std::advance(it, dis(mt));
            auto v = *it;

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[v].begin(),
                    tl[v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = 0u;
            if(compatible_colours.empty()) {
                col = c.n_colours;
            } else {
                std::uniform_int_distribution<uint32_t> cdis(0, compatible_colours.size() - 1);
                col = compatible_colours[cdis(mt)];
            }

            c.colour_vertex(v, col);

            if(tl.find(v) == tl.end()) { tl[v] = std::vector<TabuMove>{}; }
            tl[v].emplace_back(c.id[col], iteration_number);
        }
    }

    void InsertRandomVertexInBiggestColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            std::uniform_int_distribution<uint32_t> dis(0, g.p[p].size() - 1);
            auto it = g.p[p].begin();
            std::advance(it, dis(mt));
            auto v = *it;

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[v].begin(),
                    tl[v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = compatible_colours.empty() ?
                c.n_colours :
                *std::max_element(compatible_colours.begin(), compatible_colours.end(), [&] (const auto& col1, const auto& col2) { return c.colours[col1].size() < c.colours[col2].size(); });

            c.colour_vertex(v, col);
            if(tl.find(v) == tl.end()) { tl[v] = std::vector<TabuMove>{}; }
            tl[v].emplace_back(c.id[col], iteration_number);
        }
    }

    void InsertRandomVertexInSmallestColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            std::uniform_int_distribution<uint32_t> dis(0, g.p[p].size() - 1);
            auto it = g.p[p].begin();
            std::advance(it, dis(mt));
            auto v = *it;

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[v].begin(),
                    tl[v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = compatible_colours.empty() ?
                c.n_colours :
                *std::min_element(compatible_colours.begin(), compatible_colours.end(), [&] (const auto& col1, const auto& col2) { return c.colours[col1].size() < c.colours[col2].size(); });

            c.colour_vertex(v, col);
            if(tl.find(v) == tl.end()) { tl[v] = std::vector<TabuMove>{}; }
            tl[v].emplace_back(c.id[col], iteration_number);
        }
    }

    void InsertLowestDegreeVertexInRandomColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            uint32_t best_v = g.n_vertices;
            uint32_t best_ext_deg = g.n_vertices;

            for(const auto& v : g.p[p]) {
                auto w = g.vertex_by_id(v);
                auto deg = out_degree(*w, g.g);

                // Do not count the (g.p[p].size() - 1) cliqueised vertices
                auto ext_deg = deg - g.p[p].size() + 1;

                if(ext_deg < best_ext_deg) {
                    best_v = v;
                    best_ext_deg = ext_deg;
                }
            }

            assert(best_v != g.n_vertices);

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(best_v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[best_v].begin(),
                    tl[best_v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = 0u;
            if(compatible_colours.empty()) {
                col = c.n_colours;
            } else {
                std::uniform_int_distribution<uint32_t> cdis(0, compatible_colours.size() - 1);
                col = compatible_colours[cdis(mt)];
            }

            c.colour_vertex(best_v, col);
            if(tl.find(best_v) == tl.end()) { tl[best_v] = std::vector<TabuMove>{}; }
            tl[best_v].emplace_back(c.id[col], iteration_number);
        }
    }

    void InsertLowestDegreeVertexInBiggestColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            uint32_t best_v = g.n_vertices;
            uint32_t best_ext_deg = g.n_vertices;

            for(const auto& v : g.p[p]) {
                auto w = g.vertex_by_id(v);
                auto deg = out_degree(*w, g.g);

                // Do not count the (g.p[p].size() - 1) cliqueised vertices
                auto ext_deg = deg - g.p[p].size() + 1;

                if(ext_deg < best_ext_deg) {
                    best_v = v;
                    best_ext_deg = ext_deg;
                }
            }

            assert(best_v != g.n_vertices);

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(best_v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[best_v].begin(),
                    tl[best_v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = compatible_colours.empty() ?
                c.n_colours :
                *std::max_element(compatible_colours.begin(), compatible_colours.end(), [&] (const auto& col1, const auto& col2) { return c.colours[col1].size() < c.colours[col2].size(); });

            c.colour_vertex(best_v, col);
            if(tl.find(best_v) == tl.end()) { tl[best_v] = std::vector<TabuMove>{}; }
            tl[best_v].emplace_back(c.id[col], iteration_number);
        }
    }

    void InsertLowestDegreeVertexInSmallestColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            uint32_t best_v = g.n_vertices;
            uint32_t best_ext_deg = g.n_vertices;

            for(const auto& v : g.p[p]) {
                auto w = g.vertex_by_id(v);
                auto deg = out_degree(*w, g.g);

                // Do not count the (g.p[p].size() - 1) cliqueised vertices
                auto ext_deg = deg - g.p[p].size() + 1;

                if(ext_deg < best_ext_deg) {
                    best_v = v;
                    best_ext_deg = ext_deg;
                }
            }

            assert(best_v != g.n_vertices);

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(best_v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[best_v].begin(),
                    tl[best_v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = compatible_colours.empty() ?
                c.n_colours :
                *std::min_element(compatible_colours.begin(), compatible_colours.end(), [&] (const auto& col1, const auto& col2) { return c.colours[col1].size() < c.colours[col2].size(); });

            c.colour_vertex(best_v, col);
            if(tl.find(best_v) == tl.end()) { tl[best_v] = std::vector<TabuMove>{}; }
            tl[best_v].emplace_back(c.id[col], iteration_number);
        }
    }

    void InsertLowestColourDegreeVertexInRandomColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            uint32_t best_v = 0;
            uint32_t best_cdeg = g.n_vertices;

            for(const auto& v : g.p[p]) {
                auto w = g.vertex_by_id(v);
                auto cdeg = 0u;

                for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                    auto t = target(*it.first, g.g);
                    auto s = g.g[t].id;

                    if(c.partition_for[s] == p) { continue; }
                    if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                    cdeg++;
                }

                if(cdeg < best_cdeg) {
                    best_v = v;
                    best_cdeg = cdeg;
                }
            }

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(best_v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[best_v].begin(),
                    tl[best_v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = 0u;
            if(compatible_colours.empty()) {
                col = c.n_colours;
            } else {
                std::uniform_int_distribution<uint32_t> cdis(0, compatible_colours.size() - 1);
                col = compatible_colours[cdis(mt)];
            }

            c.colour_vertex(best_v, col);
            if(tl.find(best_v) == tl.end()) { tl[best_v] = std::vector<TabuMove>{}; }
            tl[best_v].emplace_back(c.id[col], iteration_number);
        }
    }

    void InsertLowestColourDegreeVertexInBiggestColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            uint32_t best_v = 0;
            uint32_t best_cdeg = g.n_vertices;

            for(const auto& v : g.p[p]) {
                auto w = g.vertex_by_id(v);
                auto cdeg = 0u;

                for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                    auto t = target(*it.first, g.g);
                    auto s = g.g[t].id;

                    if(c.partition_for[s] == p) { continue; }
                    if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                    cdeg++;
                }

                if(cdeg < best_cdeg) {
                    best_v = v;
                    best_cdeg = cdeg;
                }
            }

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(best_v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[best_v].begin(),
                    tl[best_v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = compatible_colours.empty() ?
                c.n_colours :
                *std::max_element(compatible_colours.begin(), compatible_colours.end(), [&] (const auto& col1, const auto& col2) { return c.colours[col1].size() < c.colours[col2].size(); });

            c.colour_vertex(best_v, col);
            if(tl.find(best_v) == tl.end()) { tl[best_v] = std::vector<TabuMove>{}; }
            tl[best_v].emplace_back(c.id[col], iteration_number);
        }
    }

    void InsertLowestColourDegreeVertexInSmallestColour::operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const {
        auto uncoloured_partitions = c.uncoloured_partitions;
        for(const auto& p : uncoloured_partitions) {
            uint32_t best_v = 0;
            uint32_t best_cdeg = g.n_vertices;

            for(const auto& v : g.p[p]) {
                auto w = g.vertex_by_id(v);
                auto cdeg = 0u;

                for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                    auto t = target(*it.first, g.g);
                    auto s = g.g[t].id;

                    if(c.partition_for[s] == p) { continue; }
                    if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                    cdeg++;
                }

                if(cdeg < best_cdeg) {
                    best_v = v;
                    best_cdeg = cdeg;
                }
            }

            std::vector<uint32_t> compatible_colours;
            compatible_colours.reserve(c.n_colours);

            for(auto col = 0u; col < c.n_colours; col++) {
                if(std::any_of(
                    c.colours[col].begin(),
                    c.colours[col].end(),
                    [&] (const auto& w) { return g.connected_by_original_id(best_v, w); }
                )) { continue; }

                if(std::any_of(
                    tl[best_v].begin(),
                    tl[best_v].end(),
                    [&] (const auto& tm) { return tm.colour_id == c.id[col]; }
                )) { continue; }

                compatible_colours.push_back(col);
            }

            uint32_t col = compatible_colours.empty() ?
                c.n_colours :
                *std::min_element(compatible_colours.begin(), compatible_colours.end(), [&] (const auto& col1, const auto& col2) { return c.colours[col1].size() < c.colours[col2].size(); });

            c.colour_vertex(best_v, col);
            if(tl.find(best_v) == tl.end()) { tl[best_v] = std::vector<TabuMove>{}; }
            tl[best_v].emplace_back(c.id[col], iteration_number);
        }
    }
}