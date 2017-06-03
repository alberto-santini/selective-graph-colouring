#include "mp_solution.hpp"

namespace sgcp {
    bool MpSolution::is_integer() const {
        for(const auto& c : columns) {
            if(c.second > eps && c.second < 1 - eps) { return false; }
        }
        return true;
    }
    
    ActiveColumnsWithCoeff MpSolution::active_columns_by_id(const ColumnPool& c) const {
        std::map<uint32_t, float> ac;
        
        for(const auto& col : columns) {
            if(col.second > eps) {
                auto it = std::find(c.begin(), c.end(), col.first);
                assert(it != c.end());
                
                ac[std::distance(c.begin(), it)] = col.second;
            }
        }
        
        return ac;
    }
}