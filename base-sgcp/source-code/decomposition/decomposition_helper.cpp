#include "decomposition_helper.hpp"

namespace sgcp {
    std::ostream& operator<<(std::ostream& out, const PartitionsIdVec& v) {
        out << "{ ";
        for(auto k : v) { out << k << " "; }
        out << "} ";
        return out;
    }
    
    std::ostream& operator<<(std::ostream& out, const PartitionsIdSet& s) {
        out << "{ ";
        for(auto k : s) { out << k << " "; }
        out << "} ";
        return out;
    }
}