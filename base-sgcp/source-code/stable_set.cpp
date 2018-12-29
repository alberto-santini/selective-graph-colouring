#include "stable_set.hpp"

namespace sgcp {
    StableSet::StableSet(const Graph& g) : g{g}, intersects_partition{g.n_partitions}, s{}, dummy{true} {
        for(auto v = 0u; v < g.n_vertices; v++) { s.insert(v); }
        for(auto k = 0u; k < g.n_partitions; k++) { intersects_partition[k] = true; }
    }

    bool StableSet::is_valid(bool print_details) const {
        // Always conisder the dummy stable set valid.
        if(dummy) { return true; }

        for(auto outer_it = s.begin(); outer_it != s.end(); ++outer_it) {
            auto vi = g.get().vertex_by_id(*outer_it);
            if(!vi) {
                if(print_details) {
                    std::cerr << "Vertex " << *outer_it << " is not a vertex in the graph!" << std::endl;
                }
                return false;
            }

            for(auto inner_it = std::next(outer_it); inner_it != s.end(); ++inner_it) {
                auto vj = g.get().vertex_by_id(*inner_it);
                if(!vj) {
                    if(print_details) {
                        std::cerr << "Vertex " << *inner_it << " is not a vertex in the graph!" << std::endl;
                    }
                    return false;
                }

                if(edge(*vi, *vj, g.get().g).second) {
                    if(print_details) {
                        std::cerr << "Vertices " << *outer_it << " and " << *inner_it << " are in the same stable set, but are connected by an edge!" << std::endl;
                    }
                    return false;
                }
            }
        }

        return true;
    }

    void StableSet::create_bitset() {
        for(auto k = 0u; k < g.get().n_partitions; k++) {
            intersects_partition[k] = intersects(g.get().p[k]);
        }
    }

    bool StableSet::intersects(const std::unordered_set<uint32_t>& p) const {
        // The dummy set intersects everything! :-)
        // Therefore, we short-circuit the check.
        if(dummy) { return true; }

        return std::any_of(
            s.begin(),
            s.end(),
            [&] (uint32_t i) -> bool {
                return std::find(p.begin(), p.end(), i) != p.end();
            }
        );
    }

    bool StableSet::intersects(uint32_t k) const {
        return intersects_partition[k];
    }

    uint32_t StableSet::any_common_vertex(const std::unordered_set<uint32_t>& p) const {
        auto it = std::find_if(
            s.begin(),
            s.end(),
            [&] (uint32_t i) -> bool {
                return std::find(p.begin(), p.end(), i) != p.end();
            }
        );

        assert(it != s.end());
        return *it;
    }

    uint32_t StableSet::any_common_vertex(uint32_t k) const {
        return any_common_vertex(g.get().p[k]);
    }

    float StableSet::reduced_cost(const std::vector<float>& duals) const {
        float cost = 0;

        for(auto k = 0u; k < g.get().n_partitions; k++) {
            if(intersects(g.get().p[k])) { cost += duals.at(k); }
        }

        return cost;
    }

    bool operator<(const StableSet& lhs, const StableSet& rhs) {
        auto it1 = lhs.s.begin(), it2 = rhs.s.begin();

        while(it1 != lhs.s.end()) {
            if(it2 == rhs.s.end() || *it2 < *it1) { return false; }
            else if (*it1 < *it2 ) { return true; }
            ++it1; ++it2;
        }

        return it2 != rhs.s.end();
    }

    bool operator==(const StableSet& lhs, const StableSet& rhs) {
        if(lhs.s.size() != rhs.s.size()) { return false; }

        auto it1 = lhs.s.begin(), it2 = rhs.s.begin();
        while(it1 != lhs.s.end()) {
            if(*it1 != *it2) { return false; }
            ++it1; ++it2;
        }

        assert(it2 == rhs.s.end());

        return true;
    }

    std::ostream& operator<<(std::ostream& out, const StableSet& s) {
        out << "{ ";
        for(const auto& v : s.s) { out << v << " "; }
        out << "}";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const StableSetCollection& s) {
        out << "Colouring:" << std::endl;
        uint32_t col = 0u;
        for(const auto& ss : s) {
            out << col++ << ": " << ss << " (valid? " << std::boolalpha << ss.is_valid(false) << ")" << std::endl;
        }
        return out;
    }
}
