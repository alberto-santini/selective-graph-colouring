//
// Created by alberto on 01/10/18.
//

#include "graph_weighted.h"
#include <fstream>
#include <as/repeat.h>
#include <as/graph.h>

#ifndef IL_STD
    #define IL_STD
#endif
#include <ilcplex/ilocplex.h>

namespace smwgcp_cliques {
    namespace {
        void add_partition_cliques(ClusteredWeightedGraph& graph) {
            for(const auto& cluster : graph[boost::graph_bundle].clusters) {
                for(auto i = 0u; i < cluster.size(); ++i) {
                    for(auto j = i + 1; j < cluster.size(); ++j) {
                        auto vi = cluster[i], vj = cluster[j];
                        if(!boost::edge(vi, vj, graph).second) {
                            boost::add_edge(vi, vj, graph);
                        }
                    }
                }
            }
        }

        // True iff the first vertex has larger weight than the second vertex.
        // In case of ties, break them with the vertex id.
        template<typename V, typename G>
        bool vertex_order(const V& v1, const V& v2, const G& graph) {
            if(graph[v1].weight > graph[v2].weight) {
                return true;
            } else if(graph[v1].weight == graph[v2].weight) {
                return v1 > v2;
            } else {
                return false;
            }
        }

        bool is_simplicial_pair(std::size_t v1, std::size_t v2, std::size_t w1, std::size_t w2, const DirectedGraph& dgraph) {
            const auto v1first = vertex_order(v1, v2, dgraph);
            const auto w1first = vertex_order(w1, w2, dgraph);

            const auto e_src = v1first ? v1 : v2;
            const auto e_trg = v1first ? v2 : v1;
            const auto f_src = w1first ? w1 : w2;
            const auto f_trg = w1first ? w2 : w1;

            const auto [e, e_present] = boost::edge(e_src, e_trg, dgraph);
            const auto [f, f_present] = boost::edge(f_src, f_trg, dgraph);

            (void) e_present, (void) f_present;

            assert(e_present && f_present);

            if(boost::source(e, dgraph) == boost::source(f, dgraph)) {
                const auto et = boost::target(e, dgraph);
                const auto ft = boost::target(f, dgraph);

                if(boost::edge(et, ft, dgraph).second || boost::edge(ft, et, dgraph).second) {
                    return true;
                }
            }

            return false;
        }
    }

    float sum_of_weights(const ClusteredWeightedGraph& cwgraph) {
        return std::accumulate(cwgraph[boost::graph_bundle].cluster_weights.begin(), cwgraph[boost::graph_bundle].cluster_weights.end(), 0.0f);
    }

    ClusteredWeightedGraph read_clustered_weighted_graph(std::string graph_file) {
        std::ifstream ifs(graph_file);

        if(ifs.fail()) {
            std::fprintf(stderr, "Cannot read graph file!\n");
            std::exit(1);
        }

        std::size_t num_vertices, num_edges, num_clusters;

        if(!(ifs >> num_vertices)) {
            std::fprintf(stderr, "Cannot read the number of vertices.\n");
            std::exit(1);
        }

        if(!(ifs >> num_edges)) {
            std::fprintf(stderr, "Cannot read the number of edges.\n");
            std::exit(1);
        }

        if(!(ifs >> num_clusters)) {
            std::fprintf(stderr, "Cannot read the number of clusters.\n");
            std::exit(1);
        }

        ClusteredWeightedGraph graph;

        as::repeat(num_vertices, [&] () {
            boost::add_vertex(ClusteredVertexProperties(), graph);
        });

        std::vector<float> cluster_weights;

        as::repeat(num_clusters, [&] {
            float weight;

            if(!(ifs >> weight)) {
                std::fprintf(stderr, "Cannot read a vertex weight.\n");
                std::exit(1);
            }

            cluster_weights.push_back(weight);
        });

        as::repeat(num_edges, [&] () {
            std::size_t source, target;

            if(!(ifs >> source >> target)) {
                std::fprintf(stderr, "Cannot read an edge.\n");
                std::exit(1);
            }

            boost::add_edge(source, target, graph);
        });

        std::stringstream ss;
        std::string line;

        ifs >> std::ws;

        std::vector<std::vector<std::size_t>> clusters(num_clusters);

        for(auto cluster = 0u; cluster < num_clusters; ++cluster) {
            std::getline(ifs, line);
            ss.str(line);

            std::size_t vertex;

            while(ss >> vertex) {
                graph[vertex].cluster = cluster;
                graph[vertex].weight = cluster_weights[cluster];
                clusters[cluster].push_back(vertex);
            }

            ss.str("");
            ss.clear();
        }

        graph[boost::graph_bundle] = { num_clusters, clusters, cluster_weights };
        add_partition_cliques(graph);

        return graph;
    }

    LineGraph line_graph(const ClusteredWeightedGraph &cwgraph) {
        LineGraph lgraph(boost::num_edges(cwgraph));

        std::size_t lvertex = 0u;
        for(const auto& edge : as::graph::edges(cwgraph)) {
            const auto s = boost::source(edge, cwgraph);
            const auto t = boost::target(edge, cwgraph);

            lgraph[lvertex++] = {
                s, t, std::min(cwgraph[s].weight, cwgraph[t].weight)
            };
        }

        for(auto e = 0u; e < boost::num_vertices(lgraph); ++e) {
            // Clusters of the endpoints of the first edge.
            const auto cl_e1 = cwgraph[lgraph[e].vertex1].cluster;
            const auto cl_e2 = cwgraph[lgraph[e].vertex2].cluster;

            for(auto f = e + 1; f < boost::num_vertices(lgraph); ++f) {
                // Clusters of the endpoints of the second edge.
                const auto cl_f1 = cwgraph[lgraph[f].vertex1].cluster;
                const auto cl_f2 = cwgraph[lgraph[f].vertex2].cluster;

                // If they have two endpoints in one same cluster, add the edge.
                if(cl_e1 == cl_f1 || cl_e1 == cl_f2 || cl_e2 == cl_f1 || cl_e2 == cl_f2) {
                    boost::add_edge(e, f, lgraph);
                }
            }
        }

        return lgraph;
    }

    DirectedGraph directed_acyclic(const ClusteredWeightedGraph& cwgraph) {
        return as::graph::acyclic_orientation(cwgraph, [&] (const auto& v1, const auto& v2) -> bool { return vertex_order(v1, v2, cwgraph); });
    }

    LineGraph sandwich_line_graph(const ClusteredWeightedGraph& cwgraph) {
        const auto lgraph = line_graph(cwgraph);
        const auto dgraph = directed_acyclic(cwgraph);
        LineGraph slgraph(boost::num_vertices(lgraph));

        for(auto e = 0u; e < boost::num_vertices(slgraph); ++e) {
            slgraph[e] = lgraph[e];

            // The two endpoints of the first edge.
            const auto e_vertex_1 = lgraph[e].vertex1;
            const auto e_vertex_2 = lgraph[e].vertex2;

            for(auto f = e + 1; f < boost::num_vertices(slgraph); ++f) {
                // The two endpoints of the second edge.
                const auto f_vertex_1 = lgraph[f].vertex1;
                const auto f_vertex_2 = lgraph[f].vertex2;

                // Add an edge if there was an edge in the line graph, and
                // the pair is not simplicial.
                if( boost::edge(e, f, lgraph).second &&
                    ! is_simplicial_pair(e_vertex_1, e_vertex_2, f_vertex_1, f_vertex_2, dgraph)
                ) {
                    boost::add_edge(e, f, slgraph);
                }
            }
        }

        return slgraph;
    }

    LineGraph complementary_sandwich_line_graph(const ClusteredWeightedGraph &cwgraph) {
        const auto slgraph = sandwich_line_graph(cwgraph);
        return as::graph::complementary(slgraph);
    }

    std::pair<float, float> solve_with_mip(const ClusteredWeightedGraph& cwgraph, float timeout) {
        IloEnv env;
        IloModel model(env);

        const auto n = boost::num_vertices(cwgraph);
        const auto k = cwgraph[boost::graph_bundle].num_clusters;

        IloArray<IloNumVarArray> x(env, n);
        for(auto v = 0u; v < n; ++v) {
            //x[v] = IloNumVarArray(env, k, 0, 1, IloNumVar::Bool);

            x[v] = IloNumVarArray(env, k);
            for(auto c = 0u; c < k; ++c) {
                x[v][c] = IloNumVar(env, 0, 1, IloNumVar::Bool, ("x" + std::to_string(v) + std::to_string(c)).c_str());
            }
        }
        // IloNumVarArray z(env, k, 0, IloInfinity, IloNumVar::Float);
        IloNumVarArray z(env, k);
        for(auto c = 0u; c < k; ++c) {
            z[c] = IloNumVar(env, 0, IloInfinity, IloNumVar::Float, ("z" + std::to_string(c)).c_str());
        }

        IloExpr expr(env);
        for(auto c = 0u; c < k; ++c) {
            expr += z[c];
        }
        model.add(IloObjective(env, expr));
        expr.clear();

        for(auto c = 0u; c < k; ++c) {
            for(auto d = 0u; d < k; ++d) {
                for(const auto& v : cwgraph[boost::graph_bundle].clusters[c]) {
                    expr += x[v][d];
                }
            }
            model.add(expr == 1);
            expr.clear();
        }
        expr.end();

        for(const auto& e : as::graph::edges(cwgraph)) {
            const auto s = boost::source(e, cwgraph);
            const auto t = boost::target(e, cwgraph);

            for(auto c = 0u; c < k; ++c) {
                model.add(x[s][c] + x[t][c] <= 1);
            }
        }

        for(auto v = 0u; v < n; ++v) {
            for(auto c = 0u; c < k; ++c) {
                model.add(z[c] >= cwgraph[v].weight * x[v][c]);
            }
        }

        IloCplex cplex(model);
        cplex.setParam(IloCplex::TiLim, timeout);

        try {
            if(cplex.solve()) {
                return std::make_pair(cplex.getBestObjValue(), cplex.getObjValue());
            } else {
                std::fprintf(stderr, "Error when solving the model. Saving to error.lp.\n");
                cplex.exportModel("error.lp");
                return std::make_pair(-1.0f, -1.0f);
            }
        } catch(IloException& e) {
            std::fprintf(stderr, "Exception when solving the model.\n");
            env.end();
            throw;
        }

        // Should quit this function via another exit path.
        assert(false);
        return std::make_pair(-1.0f, -1.0f);
    }
}