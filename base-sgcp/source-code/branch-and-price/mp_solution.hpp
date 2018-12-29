#ifndef _MP_SOLUTION_HPP
#define _MP_SOLUTION_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"
#include "column_pool.hpp"

#include <set>
#include <map>
#include <vector>

namespace sgcp {
    using ActiveColumnsWithCoeff = std::map<uint32_t, float>;

    struct MpSolution {
        float obj_value;
        std::map<StableSet, float> columns;
        std::vector<float> duals;

        static constexpr float eps = 1e-6;

        MpSolution(double obj_value, std::map<StableSet, float> columns, std::vector<float> duals)
            : obj_value{static_cast<float>(obj_value)}, columns{columns}, duals{duals} {}

        bool is_integer() const;
        ActiveColumnsWithCoeff active_columns_by_id(const ColumnPool& c) const;
    };
}

#endif