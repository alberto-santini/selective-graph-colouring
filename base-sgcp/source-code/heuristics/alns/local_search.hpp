#ifndef _LOCAL_SEARCH_HPP
#define _LOCAL_SEARCH_HPP

#include "../../graph.hpp"
#include "alns_colouring.hpp"

namespace sgcp {
    // This class describes a generic local search operator, that is, an operator
    // that - given a colouring - (fully) explores a small neighbourhood of the
    // colouring, attempting to improve it.
    class LocalSearchOperator {
    protected:
        const Graph& g;
        
    public:
        // Constructor.
        LocalSearchOperator(const Graph& g) : g{g} {}
        
        // Performs the local search on c, producing a new colouring (which
        // can coincide with the starting one).
        virtual ALNSColouring attempt_local_search(const ALNSColouring& c) const = 0;
        
        virtual ~LocalSearchOperator() {}
    };
    
    // This local search operator tries to decrease the colouring by one colour.
    // To do so, it tries to empty the smallest colour in the following way: for
    // each vertex v (of cluster P) coloured by the smallest colour, it finds a
    // colour C_i such that it's possible to colour P (its vertex w) with C_i and
    // relocate every other cluster coloured by C_i that would be incompatible
    // with w, by colouring said clusters with other colours.
    class DecreaseByOneColourLocalSearch : public LocalSearchOperator {
        void try_to_colour(ALNSColouring& n, uint32_t p) const;
        bool try_to_recolour(ALNSColouring& n, uint32_t i, uint32_t q, uint32_t v) const;
        bool try_to_move(ALNSColouring& n, uint32_t i, uint32_t q) const;
        std::vector<uint32_t> partitions_not_compatible_with(const ALNSColouring& n, uint32_t i, uint32_t v) const;
        
    public:
        DecreaseByOneColourLocalSearch(const Graph& g) : LocalSearchOperator{g} {}
        ~DecreaseByOneColourLocalSearch() {}
        ALNSColouring attempt_local_search(const ALNSColouring& c) const;
    };
}

#endif