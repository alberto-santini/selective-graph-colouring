#include "sewell_mwss_solver.hpp"
#include "../utils/console_output.hpp"
#include "../utils/dbg_output.hpp"

#include <numeric>

namespace sgcp {
    SewellMwssSolver::SewellMwssSolver(const Graph& o, const Graph& g, WeightMap w) : o{o}, g{g}, w{w} {
        assert(w.size() == g.n_vertices);
        calculate_int_weights(w);
    }

    boost::optional<StableSet> SewellMwssSolver::solve() const {
        MWSSgraph m_graph;
        MWSSdata m_data;
        wstable_info m_info;
        wstable_parameters m_params;

        auto m_weight_lower_bound = multiplier;
        auto m_weight_goal = MWISNW_MAX;

        int m_graph_allocated = 0, m_initialised = 0, m_called = 0;

        boost::optional<StableSet> s;

        reset_pointers(&m_graph, &m_data, &m_info);
        default_parameters(&m_params);

        m_graph_allocated = allocate_graph(&m_graph, g.n_vertices);
        MWIScheck_rval(m_graph_allocated, "Cannot allocate m_graph");

        m_graph.n_nodes = g.n_vertices;

        for(auto i = 1; i <= m_graph.n_nodes; i++) {
            m_graph.weight[i] = int_weights.at(i - 1);
            for(auto j = 1; j <= m_graph.n_nodes; j++) { m_graph.adj[i][j] = 0; }
        }

        assert(std::all_of(
            &m_graph.weight[1],
            &m_graph.weight[m_graph.n_nodes],
            [] (auto weight) { return weight >= 0; }
        ));

        for(auto eit = edges(g.g); eit.first != eit.second; ++eit.first) {
            auto source_id = g.g[source(*eit.first, g.g)].id + 1;
            auto target_id = g.g[target(*eit.first, g.g)].id + 1;
            m_graph.adj[source_id][target_id] = 1;
            m_graph.adj[target_id][source_id] = 1;
        }

        // build_graph fills in:
        //  * m_graph.n_edges
        //  * m_graph.edge_list
        //  * m_graph.adj_last
        //  * m_graph.node_list[i].adjacent
        //  * m_graph.node_list[i].name
        //  * m_graph.node_list[i].degree
        //  * m_graph.node_list[i].adjv
        //  * m_graph.node_list[i].adj2
        // (see wstable.c:1562)
        build_graph(&m_graph);

        // Checks consistency of the internal variables of m_graph
        assert(check_graph(&m_graph) == 1);

        SUPPRESS_OUTPUT(m_initialised = initialize_max_wstable(&m_graph, &m_info);)
        
        MWIScheck_rval(m_initialised, "Cannot initialise max wstable");

        SUPPRESS_OUTPUT(m_called = call_max_wstable(&m_graph, &m_data, &m_params, &m_info, m_weight_goal, m_weight_lower_bound);)

        if(m_called != 0) {
            free_max_wstable(&m_graph, &m_data, &m_info);
            return boost::none;
        }

        // DEBUG_ONLY(std::cout << "Sewell MWSS Solution: " << static_cast<float>(m_data.best_z) / static_cast<float>(multiplier) << std::endl;)

        s = make_stable_set(m_data);

        CLEANUP: free_max_wstable(&m_graph, &m_data, &m_info);

        return s;
    }

    StableSet SewellMwssSolver::make_stable_set(const MWSSdata& m_data) const {
        VertexIdSet s;

        for(auto i = 1; i <= m_data.n_best; i++) {
            if(m_data.best_sol[i] != NULL) {
                auto v = g.vertex_by_id(m_data.best_sol[i]->name - 1);
                assert(v);
                for(auto orig_id : g.g[*v].represented_vertices) { s.insert(orig_id); }
            }
        }

        return StableSet{o, s};
    }

    void SewellMwssSolver::calculate_int_weights(const WeightMap& w) {
        std::vector<uint32_t> vertices_id(w.size());
        std::transform(w.begin(), w.end(), vertices_id.begin(), [] (auto kv) { return kv.first; });
        std::sort(vertices_id.begin(), vertices_id.end());

        std::vector<uint32_t> vertices_id_check(w.size());
        std::iota(vertices_id_check.begin(), vertices_id_check.end(), 0u);

        assert(vertices_id == vertices_id_check);

        multiplier = o.params.mwss_multiplier;

        int_weights = std::vector<uint32_t>(w.size());
        std::transform(vertices_id.begin(), vertices_id.end(), int_weights.begin(), [&] (auto i) { return multiplier * w.at(i); });

        assert(std::all_of(
            int_weights.begin(),
            int_weights.end(),
            [] (auto weight) { return weight < std::numeric_limits<int>::max(); }
        ));
    }
}