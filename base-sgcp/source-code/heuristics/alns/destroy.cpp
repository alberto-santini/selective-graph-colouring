#include "destroy.hpp"
#include <vector>

namespace sgcp {
    void RemoveRandomVertexInRandomColour::operator()(ALNSColouring& c) const {
        std::uniform_int_distribution<uint32_t> cdis(0, c.n_colours - 1);
        auto colour = cdis(mt);
        std::uniform_int_distribution<uint32_t> vdis(0, c.colours[colour].size() - 1);
        auto v = c.colours[colour][vdis(mt)];
        c.uncolour_vertex(v);
    }

    void RemoveRandomVertexInSmallestColour::operator()(ALNSColouring& c) const {
        auto colour_it = std::min_element(c.colours.begin(), c.colours.end(), [](const auto& cv1, const auto& cv2) { return cv1.size() < cv2.size(); });
        std::uniform_int_distribution<uint32_t> vdis(0, colour_it->size() - 1);
        auto v = (*colour_it)[vdis(mt)];
        c.uncolour_vertex(v);
    }

    void RemoveRandomVertexInBiggestColour::operator()(ALNSColouring& c) const {
        auto colour_it = std::max_element(c.colours.begin(), c.colours.end(), [](const auto& cv1, const auto& cv2) { return cv1.size() < cv2.size(); });
        std::uniform_int_distribution<uint32_t> vdis(0, colour_it->size() - 1);
        auto v = (*colour_it)[vdis(mt)];
        c.uncolour_vertex(v);
    }

    void RemoveVertexWithSmallestDegree::operator()(ALNSColouring& c) const {
        uint32_t best_v = 0;
        uint32_t best_ext_deg = g.n_vertices;

        for(const auto& v : c.coloured_vertices) {
            auto w = g.vertex_by_id(v);
            auto deg = out_degree(*w, g.g);
            auto p = c.partition_for[v];
            auto ext_deg = deg - g.p[p].size() - 1;

            if(ext_deg < best_ext_deg) {
                best_v = v;
                best_ext_deg = ext_deg;
            }
        }

        c.uncolour_vertex(best_v);
    }

    void RemoveVertexWithBiggestDegree::operator()(ALNSColouring& c) const {
        uint32_t best_v = 0;
        uint32_t best_ext_deg = 0;

        for(const auto& v : c.coloured_vertices) {
            auto w = g.vertex_by_id(v);
            auto deg = out_degree(*w, g.g);
            auto p = c.partition_for[v];
            auto ext_deg = deg - g.p[p].size() - 1;

            if(ext_deg > best_ext_deg) {
                best_v = v;
                best_ext_deg = ext_deg;
            }
        }

        c.uncolour_vertex(best_v);
    }

    void RemoveVertexWithSmallestColourDegree::operator()(ALNSColouring& c) const {
        uint32_t best_v = 0;
        uint32_t best_cdeg = g.n_vertices;

        for(const auto& v : c.coloured_vertices) {
            auto w = g.vertex_by_id(v);
            auto cdeg = 0u;

            for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                auto t = target(*it.first, g.g);
                auto s = g.g[t].id;

                if(c.partition_for[s] == c.partition_for[v]) { continue; }
                if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                cdeg++;
            }

            if(cdeg < best_cdeg) {
                best_v = v;
                best_cdeg = cdeg;
            }
        }

        c.uncolour_vertex(best_v);
    }

    void RemoveVertexWithBiggestColourDegree::operator()(ALNSColouring& c) const {
        uint32_t best_v = 0;
        uint32_t best_cdeg = 0;

        for(const auto& v : c.coloured_vertices) {
            auto w = g.vertex_by_id(v);
            auto cdeg = 0u;

            for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                auto t = target(*it.first, g.g);
                auto s = g.g[t].id;

                if(c.partition_for[s] == c.partition_for[v]) { continue; }
                if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                cdeg++;
            }

            if(cdeg > best_cdeg) {
                best_v = v;
                best_cdeg = cdeg;
            }
        }

        c.uncolour_vertex(best_v);
    }

    void RemoveVertexByRouletteDegreeSmall::operator()(ALNSColouring& c) const {
        std::vector<uint32_t> inverse_degree(c.coloured_vertices.size());

        for(auto id = 0u; id < c.coloured_vertices.size(); id++) {
            auto v = c.coloured_vertices[id];
            auto w = g.vertex_by_id(v);
            auto deg = out_degree(*w, g.g);
            auto p = c.partition_for[v];
            auto ext_deg = deg - g.p[p].size() - 1;
            inverse_degree[id] = g.n_vertices - ext_deg;
        }

        uint32_t id = inverse_degree.size() - 1;
        uint32_t deg_sum = std::accumulate(inverse_degree.begin(), inverse_degree.end(), 0.0);
        std::uniform_int_distribution<uint32_t> dis(0, deg_sum);
        uint32_t deg_rnd = dis(mt);
        uint32_t deg_acc = 0;

        for(auto i = 0u; i < inverse_degree.size(); i++) {
            deg_acc += inverse_degree[i];
            if(deg_rnd <= deg_acc) { id = i; break; }
        }

        c.uncolour_vertex(c.coloured_vertices[id]);
    }

    void RemoveVertexByRouletteDegreeBig::operator()(ALNSColouring& c) const {
        std::vector<uint32_t> degree(c.coloured_vertices.size());

        for(auto id = 0u; id < c.coloured_vertices.size(); id++) {
            auto v = c.coloured_vertices[id];
            auto w = g.vertex_by_id(v);
            auto deg = out_degree(*w, g.g);
            auto p = c.partition_for[v];
            auto ext_deg = deg - g.p[p].size() - 1;
            degree[id] = ext_deg;
        }

        uint32_t id = degree.size() - 1;
        uint32_t deg_sum = std::accumulate(degree.begin(), degree.end(), 0.0);
        std::uniform_int_distribution<uint32_t> dis(0, deg_sum);
        uint32_t deg_rnd = dis(mt);
        uint32_t deg_acc = 0;

        for(auto i = 0u; i < degree.size(); i++) {
            deg_acc += degree[i];
            if(deg_rnd <= deg_acc) { id = i; break; }
        }

        c.uncolour_vertex(c.coloured_vertices[id]);
    }

    void RemoveVertexByRouletteColourDegreeSmall::operator()(ALNSColouring& c) const {
        std::vector<uint32_t> inverse_degree(c.coloured_vertices.size());

        for(auto id = 0u; id < c.coloured_vertices.size(); id++) {
            auto v = c.coloured_vertices[id];
            auto w = g.vertex_by_id(v);
            auto cdeg = 0u;

            for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                auto t = target(*it.first, g.g);
                auto s = g.g[t].id;

                if(c.partition_for[s] == c.partition_for[v]) { continue; }
                if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                cdeg++;
            }

            inverse_degree[id] = g.n_vertices - cdeg;
        }

        uint32_t id = inverse_degree.size() - 1;
        uint32_t deg_sum = std::accumulate(inverse_degree.begin(), inverse_degree.end(), 0.0);
        std::uniform_int_distribution<uint32_t> dis(0, deg_sum);
        uint32_t deg_rnd = dis(mt);
        uint32_t deg_acc = 0;

        for(auto i = 0u; i < inverse_degree.size(); i++) {
            deg_acc += inverse_degree[i];
            if(deg_rnd <= deg_acc) { id = i; break; }
        }

        c.uncolour_vertex(c.coloured_vertices[id]);
    }

    void RemoveVertexByRouletteColourDegreeBig::operator()(ALNSColouring& c) const {
        std::vector<uint32_t> degree(c.coloured_vertices.size());

        for(auto id = 0u; id < c.coloured_vertices.size(); id++) {
            auto v = c.coloured_vertices[id];
            auto w = g.vertex_by_id(v);
            auto cdeg = 0u;

            for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                auto t = target(*it.first, g.g);
                auto s = g.g[t].id;

                if(c.partition_for[s] == c.partition_for[v]) { continue; }
                if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                cdeg++;
            }

            degree[id] = cdeg;
        }

        uint32_t id = degree.size() - 1;
        uint32_t deg_sum = std::accumulate(degree.begin(), degree.end(), 0.0);
        std::uniform_int_distribution<uint32_t> dis(0, deg_sum);
        uint32_t deg_rnd = dis(mt);
        uint32_t deg_acc = 0;

        for(auto i = 0u; i < degree.size(); i++) {
            deg_acc += degree[i];
            if(deg_rnd <= deg_acc) { id = i; break; }
        }

        c.uncolour_vertex(c.coloured_vertices[id]);
    }

    void RemoveRandomColour::operator()(ALNSColouring& c) const {
        std::uniform_int_distribution<uint32_t> cdis(0, c.n_colours - 1);
        auto colour = cdis(mt);
        auto vertices = c.colours[colour];
        for(const auto& v : vertices) { c.uncolour_vertex(v); }
    }

    void RemoveSmallestColour::operator()(ALNSColouring& c) const {
        auto vertices = *std::min_element(c.colours.begin(), c.colours.end(), [](const auto& cv1, const auto& cv2) { return cv1.size() < cv2.size(); });
        for(const auto& v : vertices) { c.uncolour_vertex(v); }
    }

    void RemoveColourWithSmallestDegree::operator()(ALNSColouring& c) const {
        uint32_t best_col = 0;
        uint32_t best_col_deg = g.n_vertices * g.n_vertices;

        for(auto col = 0u; col < c.n_colours; col++) {
            uint32_t col_deg = 0;

            for(const auto& v : c.colours[col]) {
                auto w = g.vertex_by_id(v);
                auto deg = out_degree(*w, g.g);
                auto p = c.partition_for[v];
                col_deg += deg - g.p[p].size() - 1;
            }

            if(col_deg < best_col_deg) {
                best_col = col;
                best_col_deg = col_deg;
            }
        }

        auto vertices = c.colours[best_col];
        for(const auto& v : vertices) { c.uncolour_vertex(v); }
    }

    void RemoveColourWithSmallestColourDegree::operator()(ALNSColouring& c) const {
        uint32_t best_col = 0;
        uint32_t best_col_deg = g.n_vertices * g.n_vertices;

        for(auto col = 0u; col < c.n_colours; col++) {
            uint32_t col_deg = 0;

            for(const auto& v : c.colours[col]) {
                auto w = g.vertex_by_id(v);

                for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                    auto t = target(*it.first, g.g);
                    auto s = g.g[t].id;

                    if(c.partition_for[s] == c.partition_for[v]) { continue; }
                    if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                    col_deg++;
                }
            }

            if(col_deg < best_col_deg) {
                best_col = col;
                best_col_deg = col_deg;
            }
        }

        auto vertices = c.colours[best_col];
        for(const auto& v : vertices) { c.uncolour_vertex(v); }
    }

    void RemoveColourByRouletteDegreeSmall::operator()(ALNSColouring& c) const {
        std::vector<uint32_t> inverse_degree(c.n_colours);

        for(auto col = 0u; col < c.n_colours; col++) {
            uint32_t col_deg = 0;

            for(const auto& v : c.colours[col]) {
                auto w = g.vertex_by_id(v);
                auto deg = out_degree(*w, g.g);
                auto p = c.partition_for[v];
                col_deg += deg - g.p[p].size() - 1;
            }

            inverse_degree[col] = g.n_vertices * g.n_vertices - col_deg;
        }

        uint32_t id = inverse_degree.size() - 1;
        uint32_t deg_sum = std::accumulate(inverse_degree.begin(), inverse_degree.end(), 0.0);
        std::uniform_int_distribution<uint32_t> dis(0, deg_sum);
        uint32_t deg_rnd = dis(mt);
        uint32_t deg_acc = 0;

        for(auto i = 0u; i < inverse_degree.size(); i++) {
            deg_acc += inverse_degree[i];
            if(deg_rnd <= deg_acc) { id = i; break; }
        }

        auto vertices = c.colours[id];
        for(const auto& v : vertices) { c.uncolour_vertex(v); }
    }

    void RemoveColourByRouletteColourDegreeSmall::operator()(ALNSColouring& c) const {
        std::vector<uint32_t> inverse_degree(c.n_colours);

        for(auto col = 0u; col < c.n_colours; col++) {
            uint32_t col_deg = 0;

            for(const auto& v : c.colours[col]) {
                auto w = g.vertex_by_id(v);

                for(auto it = out_edges(*w, g.g); it.first != it.second; ++it.first) {
                    auto t = target(*it.first, g.g);
                    auto s = g.g[t].id;

                    if(c.partition_for[s] == c.partition_for[v]) { continue; }
                    if(std::find(c.coloured_vertices.begin(), c.coloured_vertices.end(), s) != c.coloured_vertices.end()) { continue; }

                    col_deg++;
                }
            }

            inverse_degree[col] = g.n_vertices * g.n_vertices - col_deg;
        }

        uint32_t id = inverse_degree.size() - 1;
        uint32_t deg_sum = std::accumulate(inverse_degree.begin(), inverse_degree.end(), 0.0);
        std::uniform_int_distribution<uint32_t> dis(0, deg_sum);
        uint32_t deg_rnd = dis(mt);
        uint32_t deg_acc = 0;

        for(auto i = 0u; i < inverse_degree.size(); i++) {
            deg_acc += inverse_degree[i];
            if(deg_rnd <= deg_acc) { id = i; break; }
        }

        auto vertices = c.colours[id];
        for(const auto& v : vertices) { c.uncolour_vertex(v); }
    }
}