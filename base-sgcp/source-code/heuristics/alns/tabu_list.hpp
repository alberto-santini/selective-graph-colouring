#ifndef _TABU_LIST_HPP
#define _TABU_LIST_HPP

#include <vector>
#include <unordered_map>

namespace sgcp {
    // This class represents a Tabu Move. Any vertex associated with such a
    // tabu move will not be coloured with the colour with id ``colour_id''
    // for a certain number of iterations. The iteration at which the rule
    // was created is stored in ``entry''.
    struct TabuMove {
        uint32_t colour_id;
        uint32_t entry_iteration;

        TabuMove(uint32_t colour_id, uint32_t entry_iteration) : colour_id{colour_id}, entry_iteration{entry_iteration} {}
    };

    using TabuList = std::unordered_map<uint32_t, std::vector<TabuMove>>;
}

#endif