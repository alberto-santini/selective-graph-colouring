#include "branching_helper.hpp"

#include <stdexcept>

namespace sgcp {
    uint32_t BranchingHelper::how_many_intersections(uint32_t k, const VertexIdSet& s) const {
        VertexIdSet local_vertices_intersected;

        for(auto i : s) {
            for(auto v_id : g.p[k]) {
                auto v = g.vertex_by_id(v_id);
                assert(v);

                if(g.g[*v].represents(i)) { local_vertices_intersected.insert(v_id); }
            }
        }
        return local_vertices_intersected.size();
    }

    boost::optional<uint32_t> BranchingHelper::any_vertex_in_intersection(uint32_t k, const VertexIdSet& s) const {
        for(auto i : s) {
            for(auto v_id : g.p[k]) {
                auto v = g.vertex_by_id(v_id);
                assert(v);

                if(g.g[*v].represents(i)) { return v_id; }
            }
        }
        return boost::none;
    }

    boost::optional<uint32_t> BranchingHelper::any_vertex_in_set(const VertexIdSet& s) const {
        for(auto i : s) {
            for(auto it = vertices(g.g); it.first != it.second; ++it.first) {
                Vertex v = *it.first;
                if(g.g[v].represents(i)) { return g.g[v].id; }
            }
        }
        return boost::none;
    }

    bool BranchingHelper::is_vertex_in_set(uint32_t id, const VertexIdSet& s) const {
        auto v = g.vertex_by_id(id);
        if(!v) { return false; }

        for(auto rid : g.g[*v].represented_vertices) {
            if(std::find(s.begin(), s.end(), rid) != s.end()) { return true; }
        }

        return false;
    }

    boost::optional<uint32_t> BranchingHelper::any_vertex_covered_by_exactly_one_column(uint32_t c1, uint32_t c2) const {
        if(column_pool.at(c1).dummy || column_pool.at(c2).dummy) {
            throw std::runtime_error("any_vertex_covered_by_exactly_one_column: One of the selected columns is dummy!");
        }
        
        const auto& s1 = column_pool.at(c1).get_set();
        const auto& s2 = column_pool.at(c2).get_set();

        for(auto it = vertices(g.g); it.first != it.second; ++it.first) {
            Vertex v = *it.first;
            uint32_t c = 0;

            c += (is_vertex_in_set(g.g[v].id, s1) ? 1 : 0);
            c += (is_vertex_in_set(g.g[v].id, s2) ? 1 : 0);

            if(c == 1u) { return g.g[v].id; }
        }
        return boost::none;
    }

    std::map<uint32_t, bool> BranchingHelper::mark_coloured_vertices_by_lp_solution() const {
        std::map<uint32_t, bool> coloured_vertices;

        for(auto it = vertices(g.g); it.first != it.second; ++it.first) {
            auto v_id = g.g[*it.first].id;
            const auto& v_rep = g.g[*it.first].represented_vertices;

            coloured_vertices[v_id] = false;

            for(auto cidval : sol.lp_solution_columns) {
                if(column_pool.at(cidval.first).dummy) {
                    throw std::runtime_error("mark_coloured_vertices_by_lp_solution: Base solution contains dummy column");
                }
                
                const auto& s = column_pool.at(cidval.first).get_set();
                if(std::any_of( v_rep.begin(),
                                v_rep.end(),
                                [&] (auto rep_id) {
                                    return std::find(s.begin(), s.end(), rep_id) != s.end();
                                }))
                { coloured_vertices[v_id] = true; break; }
            }
        }

        return coloured_vertices;
    }

    std::map<uint32_t, uint32_t> BranchingHelper::count_coloured_vertices_by_lp_solution_in_each_partition() const {
        auto coloured_vertices = mark_coloured_vertices_by_lp_solution();
        std::map<uint32_t, uint32_t> coloured_partitions;

        for(auto k = 0u; k < g.n_partitions; k++) {
            coloured_partitions[k] = 0u;

            for(auto v_id : g.p[k]) {
                if(coloured_vertices[v_id]) { coloured_partitions[k]++; }
            }
        }

        // There can't be any uncoloured partition
        assert(std::none_of(coloured_partitions.begin(), coloured_partitions.end(), [&] (const auto& kn) { return kn.second == 0u; }));

        return coloured_partitions;
    }

    boost::optional<uint32_t> BranchingHelper::partition_with_more_than_one_coloured_vertex() const {
        // First, count how many vertices have been coloured in each partition.
        auto coloured_vertices = count_coloured_vertices_by_lp_solution_in_each_partition();

        // If no partition has more than one coloured vertex, return boost::none
        if(std::all_of(coloured_vertices.begin(), coloured_vertices.end(), [&] (const auto& kn) { return kn.second == 1u; })) { return boost::none; }

        // I want to take the partition that has:
        // 1) More coloured vertices [coloured_vertices.at(k)]; breaking ties by:
        // 2) Fewer vertices overall in the partition [g.num_vertices - g.p.at(k).size()]; breaking ties by:
        // 3) Partition number [k].
        std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> col_size_part;

        for(auto k = 0u; k < g.n_partitions; k++) {
            if(coloured_vertices.at(k) > 1u) {
                uint32_t partition_antisize = g.n_vertices - g.p[k].size();
                col_size_part.push_back(std::tie(coloured_vertices.at(k), partition_antisize, k));
            }
        }

        assert(col_size_part.size() > 0u);

        auto it = std::max_element(col_size_part.begin(), col_size_part.end());
        return std::get<2>(*it);
    }

    boost::optional<uint32_t> BranchingHelper::vertex_in_most_fractional_column_that_covers_partition(uint32_t part_k) const {
        float fract_value = 0.5;
        uint32_t chosen_id = g.n_vertices;

        for(const auto& cidval : sol.lp_solution_columns) {
            const StableSet& s = column_pool.at(cidval.first);
            float fractionality = std::abs(cidval.second - 0.5f);
            
            if(s.dummy) {
                throw std::runtime_error("vertex_in_most_fractional_column_that_covers_partition: Base solution contains dummy column!");
            }

            if(how_many_intersections(part_k, s.get_set()) > 0u && fractionality < fract_value - eps) {
                fract_value = fractionality;

                auto vid = any_vertex_in_intersection(part_k, s.get_set());
                assert(vid);
                chosen_id = *vid;
            }
        }

        if(chosen_id < g.n_vertices) { return chosen_id; } else { return boost::none; }
    }

    boost::optional<std::pair<uint32_t, uint32_t>> BranchingHelper::most_fractional_vertex_in_partition_with_more_than_one_coloured_vertex() const {
        uint32_t k_id = g.n_partitions;
        uint32_t v_id = g.n_vertices;
        float fractionality = 0.0f;

        for(auto k = 0u; k < g.p.size(); k++) {
            if(g.p[k].size() == 1u) { continue; }

            std::set<uint32_t> coloured_vertices;
            uint32_t p_vertex = g.n_vertices;
            float p_fractionality = 0.0f;

            for(auto v_id : g.p[k]) {
                auto v = g.vertex_by_id(v_id);
                assert(v);
                const auto& v_rep = g.g[*v].represented_vertices;
                float v_fractionality = 0.0f;

                for(auto cidval : sol.lp_solution_columns) {
                    const auto& s = column_pool.at(cidval.first).get_set();

                    if(cidval.second > eps && std::any_of(v_rep.begin(), v_rep.end(), [&] (auto rep_id) { return std::find(s.begin(), s.end(), rep_id) != s.end(); })) {
                        coloured_vertices.insert(v_id);
                        v_fractionality += cidval.second;
                    }
                }

                if(v_fractionality > p_fractionality) {
                    p_fractionality = v_fractionality;
                    p_vertex = v_id;
                }
            }

            if(p_vertex < g.n_vertices && coloured_vertices.size() > 1 && p_fractionality > fractionality) {
                fractionality = p_fractionality;
                k_id = k;
                v_id = p_vertex;
            }
        }

        if(k_id < g.n_partitions) {
            assert(v_id < g.n_vertices);
            return std::make_pair(k_id, v_id);
        }

        return boost::none;
    }

    uint32_t BranchingHelper::most_fractional_column() const {
        float fract_value = 0.0f;
        uint32_t column1_id = column_pool.size();

        for(auto cid = 0u; cid < column_pool.size(); cid++) {
            auto it = std::find_if(
                sol.lp_solution_columns.begin(),
                sol.lp_solution_columns.end(),
                [&] (const auto& cidval) -> bool { return cidval.first == cid; }
            );

            if(it != sol.lp_solution_columns.end() && it->second < 1 - eps && it->second > fract_value + eps) {
                if(column_pool.at(cid).dummy) {
                    throw std::runtime_error("most_fractional_column: Base solution contains dummy column!");
                }
                
                fract_value = it->second;
                column1_id = cid;
            }
        }

        assert(column1_id < column_pool.size());

        return column1_id;
    }

    boost::optional<uint32_t> BranchingHelper::another_column_covering_vertex(uint32_t column1_id, uint32_t vertex_id) const {
        if(column_pool.at(column1_id).dummy) {
            throw std::runtime_error("another_column_covering_vertex: Base solution contains first column which is dummy!");
        }
        
        assert(is_vertex_in_set(vertex_id, column_pool.at(column1_id).get_set()));

        uint32_t column2_id = column_pool.size();

        for(auto cid = 0u; cid < column_pool.size(); cid++) {
            if(cid == column1_id) { continue; }

            auto it = std::find_if(
                sol.lp_solution_columns.begin(),
                sol.lp_solution_columns.end(),
                [&] (const auto& cidval) -> bool { return cidval.first == cid; }
            );

            if(it != sol.lp_solution_columns.end()) {
                if(is_vertex_in_set(vertex_id, column_pool.at(cid).get_set())) {
                    column2_id = cid;
                    break;
                }
            }
        }

        if(column2_id < column_pool.size()) {
            if(column_pool.at(column2_id).dummy) {
                throw std::runtime_error("another_column_covering_vertex: Base solution contains second column which is dummy!");
            }
            return column2_id;
        } else { return boost::none; }
    }
}