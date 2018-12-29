//
// Created by alberto on 08/05/18.
//

#include "graph.h"
#include "graph_weighted.h"
#include <as/graph.h>
#include <as/max_clique.h>
#include <as/and_die.h>
#include <as/mwis.h>
#include <ProgramOptions.hxx>
#include <fstream>
#include <chrono>
#include <optional>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

int main(int argc, char* argv[]) {
    using namespace std::chrono;
    using as::and_die;
    namespace fs = std::experimental::filesystem;

    po::parser parser;

    parser["graph"]
        .abbreviation('g')
        .description("File containing the graph. Mandatory.")
        .type(po::string);

    parser["output"]
        .abbreviation('o')
        .description("File where we append the results. Mandatory.")
        .type(po::string);

    parser["cplex-timeout"]
        .abbreviation('t')
        .description("Timeout to pass to the CPLEX solver.")
        .type(po::f32);

    parser["problem-type"]
        .abbreviation('y')
        .description("Type of problem to solve. It can be either weighted-clique, weighted-mip, or unweighted (default).")
        .type(po::string)
        .fallback("unweighted");

    parser["help"]
        .abbreviation('h')
        .description("Prints this help text.")
        .callback([&] () { std::cout << parser << "\n"; });

    parser(argc, argv);

    if(!parser["graph"].was_set()) {
        std::cerr << "You need to specify a graph file!" << and_die();
    }

    if(!parser["output"].was_set()) {
        std::cerr << "You need to specify an output file!" << and_die();
    }

    std::optional<float> cplex_timeout = std::nullopt;

    if(parser["cplex-timeout"].was_set()) {
        cplex_timeout = parser["cplex-timeout"].get().f32;
    }

    std::string graph_file = parser["graph"].get().string;
    std::string output_file = parser["output"].get().string;

    if(!fs::exists(graph_file)) {
        std::cerr << "Graph file does not exist: " << graph_file << and_die();
    }

    std::ofstream ofs;
    ofs.open(output_file, std::ios_base::app);

    if(ofs.fail()) {
        std::cerr << "Cannot access output file: " << output_file << and_die();
    }

    const auto instance = fs::path{graph_file}.stem().string();
    const auto zero_time = high_resolution_clock::now();

    if(parser["problem-type"].get().string == "unweighted") {
        std::cout << "Reading graph from file...\n";

        const auto cgraph = sgcp_cliques::read_clustered_graph(graph_file);

        std::cout << "Graph read from file (" << duration_cast<duration<float>>(high_resolution_clock::now() - zero_time).count() << " s)\n";
        std::cout << "Preparing Clique graph...\n";

        const auto working_graph = as::graph::complementary(cgraph);
        const auto clique_graph = sgcp_cliques::complementary_sandwich_line_graph(working_graph);

        std::cout << "Clique graph ready (" << duration_cast<duration<float>>(high_resolution_clock::now() - zero_time).count() << " s)\n";
        std::cout << "Launching the Clique solver...\n";

        const auto start_time = high_resolution_clock::now();
        const auto max_clique_sol = as::max_clique::solve_with_mip(clique_graph, cplex_timeout);
        const auto end_time = high_resolution_clock::now();
        const auto elapsed = duration_cast<duration<float>>(end_time - start_time).count();
        const auto chromatic_n_ub = sgcp_cliques::number_of_partitions(cgraph) - max_clique_sol.lb;
        const auto chromatic_n_lb = sgcp_cliques::number_of_partitions(cgraph) - max_clique_sol.ub;

        ofs << "unweighted," << instance << "," << cgraph << "," << chromatic_n_lb << "," << chromatic_n_ub << "," << elapsed << "\n";
    } else if(parser["problem-type"].get().string == "weighted-clique") {
        std::cout << "Reading graph from file...\n";

        const auto cwgraph = smwgcp_cliques::read_clustered_weighted_graph(graph_file);

        std::cout << "Graph read from file (" << duration_cast<duration<float>>(high_resolution_clock::now() - zero_time).count() << " s)\n";
        std::cout << "Preparing Max-Weight Clique graph...\n";

        const auto working_graph = as::graph::complementary(cwgraph);
        const auto clique_graph = smwgcp_cliques::complementary_sandwich_line_graph(working_graph);

        std::cout << "Max-Weight Clique graph ready (" << duration_cast<duration<float>>(high_resolution_clock::now() - zero_time).count() << " s) ";
        std::cout << boost::num_vertices(clique_graph) << " vertices and " << boost::num_edges(clique_graph) << " edges\n";
        std::cout << "Launching the Max-Weight Clique solver (MIP)...\n";

        const auto start_time = high_resolution_clock::now();
        const auto max_clique_sol = as::max_clique::solve_with_mip(clique_graph, cplex_timeout);
        const auto end_time = high_resolution_clock::now();
        const auto elapsed = duration_cast<duration<float>>(end_time - start_time).count();

        std::cout << "MIP Clique solver finished (" << elapsed << " s)\n";

        const auto weighted_chromatic_n_lb = smwgcp_cliques::sum_of_weights(cwgraph) - max_clique_sol.ub;
        const auto weighted_chromatic_n_ub = smwgcp_cliques::sum_of_weights(cwgraph) - max_clique_sol.lb;

        std::cout << "Clique solver result (LB): " << weighted_chromatic_n_lb << " (" << smwgcp_cliques::sum_of_weights(cwgraph) << " - " << max_clique_sol.ub << ")\n";
        std::cout << "Clique solver result (UB): " << weighted_chromatic_n_ub << " (" << smwgcp_cliques::sum_of_weights(cwgraph) << " - " << max_clique_sol.lb << ")\n";

        ofs << "weighted-clique," << instance << "," << cwgraph << "," << clique_graph << "," << weighted_chromatic_n_lb << "," << weighted_chromatic_n_ub << "," << elapsed << "\n";
    } else if(parser["problem-type"].get().string == "weighted-stable-set") {
        std::cout << "Reading graph from file...\n";

        const auto cwgraph = smwgcp_cliques::read_clustered_weighted_graph(graph_file);
        const auto working_graph = as::graph::complementary(cwgraph);

        std::cout << "Graph read from file (" << duration_cast<duration<float>>(high_resolution_clock::now() - zero_time).count() << " s)\n";
        std::cout << "Preparing Max-Weight Stable Set graph...\n";

        const auto mwss_graph = smwgcp_cliques::sandwich_line_graph(working_graph);

        std::cout << "Max-Weight Stable Set graph ready (" << duration_cast<duration<float>>(high_resolution_clock::now() - zero_time).count() << " s) ";
        std::cout << boost::num_vertices(mwss_graph) << " vertices and " << boost::num_edges(mwss_graph) << " edges\n";
        std::cout << "Launching the Max-Weight Stable Set solver (Sewell)...\n";

        std::vector<std::uint32_t> weights(boost::num_vertices(mwss_graph));
        for(auto v = 0u; v < boost::num_vertices(mwss_graph); ++v) {
            weights[v] = static_cast<std::uint32_t>(mwss_graph[v].weight);
        }

        const auto start_time = high_resolution_clock::now();
        const auto mwss_sol = as::mwis::mwis(weights, mwss_graph);
        const auto end_time = high_resolution_clock::now();
        const auto elapsed = duration_cast<duration<float>>(end_time - start_time).count();

        std::cout << "Sewell Stable Set solver finished (" << elapsed << " s)\n";

        const float stable_set_result = std::accumulate(mwss_sol.begin(), mwss_sol.end(), 0.0f,
            [&] (float cum, auto v) -> float {
                return cum + mwss_graph[v].weight;
            }
        );
        const auto weighted_chromatic_n = smwgcp_cliques::sum_of_weights(cwgraph) - stable_set_result;

        std::cout << "Stable Set solver result: " << weighted_chromatic_n << " (" << smwgcp_cliques::sum_of_weights(cwgraph) << " - " << stable_set_result << ")\n";
        ofs << "weighted-stable-set," << instance << "," << cwgraph << "," << mwss_graph << "," << weighted_chromatic_n << "," << elapsed << "\n";
    } else if(parser["problem-type"].get().string == "weighted-mip") {
        std::cout << "Reading graph from file...\n";

        const auto cwgraph = smwgcp_cliques::read_clustered_weighted_graph(graph_file);

        std::cout << "Graph read from file (" << duration_cast<duration<float>>(high_resolution_clock::now() - zero_time).count() << " s)\n";
        std::cout << "Launching the MIP solver...\n";

        const auto start_time = high_resolution_clock::now();
        const auto mip_result = smwgcp_cliques::solve_with_mip(cwgraph, cplex_timeout.value_or(3600.0f));
        const auto end_time = high_resolution_clock::now();
        const auto elapsed = duration_cast<duration<float>>(end_time - start_time).count();

        std::cout << "MIP solver finished (" << elapsed << " s)\n";
        std::cout << "MIP solver result: LB = " << mip_result.first << ", UB: " << mip_result.second << "\n";
        
        ofs << "weighted-mip," << instance << "," << cwgraph << "," << mip_result.first << "," << mip_result.second << "," << elapsed << "\n";
    } else {
        std::cerr << "Wrong problem type: " << parser["problem-type"].get().string << "\n";
        return 1;
    }
    
    return 0;
}