//
// Created by alberto on 23/04/17.
//

#include <utils/console_colour.hpp>
#include "hoshino_populator.hpp"

namespace sgcp {
    ColumnPool HoshinoPopulator::enlarge_pool() {
        using namespace Console;

        std::cout << colour_magenta("Applying Hoshino's populate method") << std::endl;

        ColumnPool new_cols;

        for(const auto& sset : cp) {
            enlarge_stable_set(sset, new_cols);
        }

        std::cout << "Hoshino's populate method generated " << new_cols.size()
                  << " new columns starting from " << cp.size()
                  << " existing ones." << std::endl;

        return new_cols;
    }

    void HoshinoPopulator::enlarge_stable_set(const StableSet &s, ColumnPool& new_cols) {
        for(auto v = 0u; v < g.n_vertices - 1; ++v) {
            // Skip dummy stable sets
            if(s.dummy) { continue; }

            // Only interested in vertices in this set!
            if(!s.has_vertex(v)) { continue; }

            auto rem_set = s;
            rem_set.remove_vertex(v);

            bool added_stg = false;

            for(auto w = v + 1; w < g.n_vertices; ++w) {
                // Not interested in re-inserting the same vertex we just removed!
                if(w == v) { continue; }

                if(std::any_of(
                    rem_set.get_set().begin(),
                    rem_set.get_set().end(),
                    [&] (uint32_t vremaining) -> bool {
                        // The partition of w is already coloured by this colour, not intersted!
                        if(g.partition_for(w) == g.partition_for(vremaining)) { return true; }

                        // Vertices linked by an edge: cannot colour together!
                        if(g.connected(w, vremaining)) { return true; }

                        return false;
                    }
                )) { continue; }

                rem_set.add_vertex(w);
                assert(rem_set.is_valid(true));

                added_stg = true;
            }

            if(added_stg && std::find(new_cols.begin(), new_cols.end(), rem_set) == new_cols.end()) {
                new_cols.push_back(rem_set);
            }
        }
    }
}