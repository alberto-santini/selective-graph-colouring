#ifndef _DECOMPOSITION_HELPER
#define _DECOMPOSITION_HELPER

#include <vector>
#include <set>
#include <map>

#include <iostream>

namespace sgcp {
    using PartitionsIdVec = std::vector<uint32_t>;
    using PartitionsIdSet = std::set<uint32_t>;
    using PartitionsVec = std::vector<std::vector<uint32_t>>;
    using PartitionsSet = std::set<std::set<uint32_t>>;
    using PartitionsCliqueVec = std::vector<std::pair<uint32_t, uint32_t>>;
    using Stack = std::vector<uint32_t>;
    
    struct StableSetIntersectsAllPartitions {};
    
    std::ostream& operator<<(std::ostream& out, const PartitionsIdVec& v);
    std::ostream& operator<<(std::ostream& out, const PartitionsIdSet& s);
}

#endif