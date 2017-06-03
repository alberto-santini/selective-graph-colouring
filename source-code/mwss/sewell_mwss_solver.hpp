#ifndef _SEWELL_MWSS_SOLVER_HPP
#define _SEWELL_MWSS_SOLVER_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"

#include <vector>
#include <boost/optional.hpp>

extern "C" {
    #include <exactcolors/mwis_sewell/mwss.h>
    #include <exactcolors/mwis_sewell/mwss_ext.h>
}

namespace sgcp {
    class SewellMwssSolver {
        // Original graph, before any branching rules modified it
        const Graph& o;
        
        const Graph& g;
        
        WeightMap w;

        std::vector<uint32_t> int_weights;
        uint32_t multiplier;

        static constexpr float eps = 1e-6;

        void calculate_int_weights(const WeightMap& w);
        StableSet make_stable_set(const MWSSgraph& m_graph, const MWSSdata& m_data) const;

    public:
        SewellMwssSolver(const Graph& o, const Graph& g, WeightMap w);

        // Solves the Maximum Weight Stable Set problem on the graph, with
        // the given weights, using the Sewell algorithm, incldued in the
        // Exactcolors package by Stephan Held.
        boost::optional<StableSet> solve() const;
    };
}

#endif