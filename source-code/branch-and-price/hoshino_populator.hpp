//
// Created by alberto on 23/04/17.
//

#ifndef SGCP_HOSHINO_POPULATOR_HPP
#define SGCP_HOSHINO_POPULATOR_HPP

#include "column_pool.hpp"

namespace sgcp {
    class HoshinoPopulator {
        const Graph& g;
        ColumnPool& cp;

        void enlarge_stable_set(const StableSet& s, ColumnPool& new_cols);

    public:
        HoshinoPopulator(const Graph& g, ColumnPool& cp) : g{g}, cp{cp} {}
        ColumnPool enlarge_pool();
    };
}

#endif //SGCP_HOSHINO_POPULATOR_HPP
