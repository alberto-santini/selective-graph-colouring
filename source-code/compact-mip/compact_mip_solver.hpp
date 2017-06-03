#ifndef _COMPACT_MIP_SOLVER_HPP
#define _COMPACT_MIP_SOLVER_HPP

#include "../graph.hpp"
#include "../stable_set.hpp"

#include <boost/optional.hpp>
#include <vector>

#ifndef IL_STD
    #define IL_STD
#endif

#include <cstring>
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN

namespace sgcp {
    class CompactMipSolver {
    private:
        const Graph& g;

        StableSetCollection make_stable_sets(const IloCplex& cplex, const IloArray<IloNumVarArray>& x, const IloNumVarArray& y) const;

    public:
        CompactMipSolver(const Graph& g) : g{g} {}

        boost::optional<StableSetCollection> solve() const;
    };
}

#endif