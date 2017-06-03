#ifndef _ACCEPTANCE_HPP
#define _ACCEPTANCE_HPP

#include "alns_colouring.hpp"

#include <vector>
#include <random>

namespace sgcp {
    // This class represnts a move acceptance criterion for ALNS. Given the
    // current solution and the new incumbent, it has to decide whether to
    // accpet the new move or not. It can use the current iteration number
    // as additional data to make the decision.
    struct ALNSAcceptance {
        std::mt19937* mt;

        virtual ~ALNSAcceptance() {};
        virtual bool operator()(const ALNSColouring& current, const ALNSColouring& incumbent, uint32_t iteration_number) const = 0;
    };
    
    // This acceptance criterion simply accepts every move (it's a random walk).
    struct AcceptEverything : public ALNSAcceptance {
        ~AcceptEverything() {}
        bool operator()(const ALNSColouring& current, const ALNSColouring& incumbent, uint32_t iteration_number) const;
    };

    // This acceptance criterion accepts every move that will not increase the number
    // of colours used. It's similar to hill climbing, except that we do not only accept
    // strictly improving moves, but also moves that leave the objective funcion unchanged.
    struct AcceptNonDeteriorating : public ALNSAcceptance {
        ~AcceptNonDeteriorating() {}
        bool operator()(const ALNSColouring& current, const ALNSColouring& incumbent, uint32_t iteration_number) const;
    };

    // This acceptance criterion accepts worsening moves with a certain probability that
    // starts at value initial_prob and decreases linearly to 0 in ``total_iterations'' iterations.
    struct WorseAccept : public ALNSAcceptance {
        float initial_prob;
        uint32_t total_iterations;
        mutable std::uniform_real_distribution<float> dis;

        WorseAccept(float initial_prob, uint32_t total_iterations)
            : initial_prob{initial_prob}, total_iterations{total_iterations}, dis{0.0, 1.0}
        { assert(initial_prob <= 1.0); assert(initial_prob >= 0.0); }
        ~WorseAccept() {}

        bool operator()(const ALNSColouring& current, const ALNSColouring& incumbent, uint32_t iteration_number) const;
    };
}

#endif