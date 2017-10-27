#include "acceptance.hpp"

namespace sgcp {
    bool AcceptEverything::operator()(
        const ALNSColouring& current [[maybe_unused]],
        const ALNSColouring& incumbent [[maybe_unused]],
        uint32_t iteration_number [[maybe_unused]]) const {
        return true;
    }

    bool AcceptNonDeteriorating::operator()(
        const ALNSColouring& current,
        const ALNSColouring& incumbent,
        uint32_t iteration_number [[maybe_unused]]) const {
        return incumbent.score() <= current.score();
    }

    bool WorseAccept::operator()(const ALNSColouring& current, const ALNSColouring& incumbent, uint32_t iteration_number) const {
        if(incumbent.score() <= current.score()) { return true; }
        return dis(*mt) < initial_prob * (float)iteration_number / (float)total_iterations;
    }
}