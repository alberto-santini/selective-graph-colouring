#ifndef _DECOMPOSITION_GRAPH_HELPER
#define _DECOMPOSITION_GRAPH_HELPER

#include "../graph.hpp"
#include "decomposition_helper.hpp"

namespace sgcp {
    struct DecompositionGraphHelper {
        const Graph& g;
        mutable PartitionsSet colourable_cache;
        
        Graph make_subgraph(const std::set<uint32_t>& partitions) const;
        bool can_be_coloured_the_same(const PartitionsIdSet& partitions) const;
        void all_maximal_independent_sets(const Graph& subgraph, Stack& s, uint32_t part_size) const;
        bool independent(const Graph& subgraph, uint32_t v_id, const std::vector<uint32_t>& other_v) const;
        bool heuristic_stable_set_covers_all_partitions(const Graph& subg, const PartitionsIdSet& partitions) const;
        uint32_t partition_external_degree(uint32_t k) const;
        
        DecompositionGraphHelper(const Graph& g) : g{g} { colourable_cache = PartitionsSet{}; }
    };
}

#endif