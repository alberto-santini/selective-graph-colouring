#ifndef _DECOMPOSITION_SOLVER_HPP
#define _DECOMPOSITION_SOLVER_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"
#include "../solver_stats.hpp"
#include "decomposition_helper.hpp"
#include "decomposition_graph_helper.hpp"
#include "decomposition_model_helper.hpp"

#include <boost/optional.hpp>

namespace sgcp {    
    class DecompositionSolver {
    public:
        enum class IncompatibleThreeCutsStrategy {
            AddAllOffline,
            AddWhenViolated
        };
        
    private:        
        const Graph& g;
        DecompositionGraphHelper gh;
        
        boost::optional<MpSolution> initial_solution;
        uint32_t ub;
        
        IncompatibleThreeCutsStrategy inc_strategy;
        PartitionsSet inc_cache;

        uint32_t get_upper_bound() const;
        PartitionsVec uncolourable_subpartitions(PartitionsIdVec partitions) const;
        PartitionsVec cached_3incompatible_cuts(const PartitionsIdVec& partitions) const;
        void cache_all_3incompatible_cuts();

    public:
        DecompositionSolver(const Graph& g);
        void solve();
    };
}

#endif