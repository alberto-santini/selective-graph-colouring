#include "decomposition_graph_helper.hpp"
#include "../mwss/sewell_mwss_solver.hpp"

namespace sgcp {
    Graph DecompositionGraphHelper::make_subgraph(const std::set<uint32_t>& partitions) const {
        BoostGraph subg;
        Partition subp(g.n_partitions);
        std::map<uint32_t, uint32_t> id_shift;
        uint32_t subid = 0u;

        for(auto it = vertices(g.g); it.first != it.second; ++it.first) {
            auto v_id = g.g[*it.first].id;
            auto k = g.partition_for(v_id);
            if(partitions.find(k) != partitions.end()) {
                auto subv = add_vertex(subg);
                id_shift[v_id] = subid;
                subg[subv] = VertexInfo{subid++, g.g[*it.first].represented_vertices};
            }
        }

        for(auto it = edges(g.g); it.first != it.second; ++it.first) {
            auto s = source(*it.first, g.g);
            auto s_id = g.g[s].id;
            auto ks = g.partition_for(s_id);
            if(partitions.find(ks) == partitions.end()) { continue; }

            auto t = target(*it.first, g.g);
            auto t_id = g.g[t].id;
            auto kt = g.partition_for(t_id);
            if(partitions.find(kt) == partitions.end()) { continue; }

            // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
            auto s_subv = boost::make_optional(false, Vertex{});
            auto t_subv = boost::make_optional(false, Vertex{});

            for(auto svit = vertices(subg); svit.first != svit.second; ++svit.first) {
                if(!s_subv && subg[*svit.first].id == id_shift[s_id]) { s_subv = *svit.first; if(t_subv) { break; } }
                if(!t_subv && subg[*svit.first].id == id_shift[t_id]) { t_subv = *svit.first; if(s_subv) { break; } }
            }
            assert(s_subv && t_subv);

            add_edge(*s_subv, *t_subv, subg);
        }

        for(auto k = 0u; k < g.n_partitions; k++) {
            subp[k] = std::unordered_set<uint32_t>();
            if(partitions.find(k) != partitions.end()) {
                for(auto v_id : g.p[k]) {
                    subp[k].insert(id_shift[v_id]);
                }
            }
        }

        return Graph{subg, subp, g.params};
    }
    
    bool DecompositionGraphHelper::can_be_coloured_the_same(const PartitionsIdSet& partitions) const {
        if(partitions.size() < 2u) { return true; }

        // 1) Hash table check: if we already established that these partitions can
        // be coloured, the return true.
        if(colourable_cache.find(partitions) != colourable_cache.end()) { return true; }

        Graph subg = make_subgraph(partitions);

        // 2) If we can heuristically find a stable set that covers all partitions,
        // then return true.
        if(heuristic_stable_set_covers_all_partitions(subg, partitions)) { return true; }

        // 3) Resort to enumerating all stable sets!
        try {
            auto it = vertices(subg.g);
            auto any_v = *it.first;
            auto any_id = subg.g[any_v].id;
            Stack s = {any_id};

            all_maximal_independent_sets(subg, s, partitions.size());
        } catch(StableSetIntersectsAllPartitions& _) {
            colourable_cache.insert(partitions);
            return true;
        }

        return false;
    }
    
    void DecompositionGraphHelper::all_maximal_independent_sets(const Graph& subgraph, Stack& s, uint32_t part_size) const {
        bool maximal = true;

        for(auto v_id = s.back() + 1; v_id < subgraph.n_vertices; v_id++) {
            if(independent(subgraph, v_id, s)) {
                s.push_back(v_id);
                all_maximal_independent_sets(subgraph, s, part_size);
                s.pop_back();
                maximal = false;
            }
        }

        if(maximal) {
            if(s.size() < part_size) { return; }

            // Here we use an exception as a quick way to exit from the
            // nested call stack.
            throw StableSetIntersectsAllPartitions{};
        }
    }

    bool DecompositionGraphHelper::independent(const Graph& subgraph, uint32_t v_id, const std::vector<uint32_t>& other_v) const {
        auto v = subgraph.vertex_by_id(v_id);
        assert(v);

        for(auto w_id : other_v) {
            auto w = subgraph.vertex_by_id(w_id);
            assert(w);

            if(edge(*v, *w, subgraph.g).second) { return false; }
        }

        return true;
    }

    bool DecompositionGraphHelper::heuristic_stable_set_covers_all_partitions(const Graph& subg, const PartitionsIdSet& partitions) const {
        WeightMap wm;
        uint32_t iter = 0u;

        for(auto i = 0u; i < subg.n_vertices; i++) { wm[i] = 1; }

        while(iter < 10) {
            auto solv = SewellMwssSolver(g, subg, wm);
            auto sset = solv.solve();

            if(!sset) { return false; }

            PartitionsIdVec uncovered_partitions;
            for(auto k : partitions) { if(!sset->intersects(k)) { uncovered_partitions.push_back(k); } }

            if(uncovered_partitions.size() == 0u) { return true; }

            for(auto k : uncovered_partitions) {
                for(auto v_id : subg.p[k]) {
                    assert(wm.find(v_id) != wm.end());
                    wm[v_id] += 1;
                }
            }

            iter++;
        }

        return false;
    }
    
    uint32_t DecompositionGraphHelper::partition_external_degree(uint32_t k) const {
        PartitionsIdSet connected_vertices;
        for(auto v_id : g.p[k]) {
            auto v = g.vertex_by_id(v_id);
            assert(v);
            
            for(auto it = out_edges(*v, g.g); it.first != it.second; ++it.first) {
                auto w_id = g.g[target(*it.first, g.g)].id;
                if(std::find(g.p[k].begin(), g.p[k].end(), w_id) == g.p[k].end()) { connected_vertices.insert(w_id); }
            }
        }        
        return connected_vertices.size();
    }
}