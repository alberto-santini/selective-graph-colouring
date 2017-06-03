#ifndef _REPAIR_HPP
#define _REPAIR_HPP

#include "../../graph.hpp"
#include "alns_colouring.hpp"
#include "tabu_list.hpp"

#include <random>

namespace sgcp {
    // A repair move takes an incomplete ALNS colouring and turns it into a
    // complete colouring, by inserting vertices of uncoloured partitions in
    // either existing or new colours.
    struct RepairMove {
        const Graph& g;
        std::mt19937& mt;

        RepairMove(const Graph& g, std::mt19937& mt) : g{g}, mt{mt} {}
        virtual ~RepairMove() {}

        virtual void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const = 0;
    };
    
    // For each uncoloured partition, it takes a random vertex of the partition
    // and inserts it in any colour where it's compatible to insert it. If there is
    // no compatible colour, it creates a new one.
    struct InsertRandomVertexInRandomColour : public RepairMove {
        InsertRandomVertexInRandomColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertRandomVertexInRandomColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };

    // For each uncoloured partition, it takes a random vertex of the partition
    // and inserts it in the compatible colour with the largest cardinality. If
    // there is no compatible colour, it creates a new one.
    struct InsertRandomVertexInBiggestColour : public RepairMove {
        InsertRandomVertexInBiggestColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertRandomVertexInBiggestColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };

    // For each uncoloured partition, it takes a random vertex of the partition
    // and inserts it in the compatible colour with the smallest cardinality. If
    // there is no compatible colour, it creates a new one.
    struct InsertRandomVertexInSmallestColour : public RepairMove {
        InsertRandomVertexInSmallestColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertRandomVertexInSmallestColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };

    // Similar to InsertRandomVertexInRandomColour, but the vertex is chosen as
    // that, among the vertices of uncoloured partitions, with the smallest
    // degree. We count in the degree of v all vertices that are connected to v,
    // and belong to a different partition.
    struct InsertLowestDegreeVertexInRandomColour : public RepairMove {
        InsertLowestDegreeVertexInRandomColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertLowestDegreeVertexInRandomColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };

    // Similar to InsertRandomVertexInBiggestColour, but the vertex is chosen as
    // that, among the vertices of uncoloured partitions, with the smallest
    // degree. We count in the degree of v all vertices that are connected to v,
    // and belong to a different partition.
    struct InsertLowestDegreeVertexInBiggestColour : public RepairMove {
        InsertLowestDegreeVertexInBiggestColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertLowestDegreeVertexInBiggestColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };

    // Similar to InsertRandomVertexInSmallestColour, but the vertex is chosen as
    // that, among the vertices of uncoloured partitions, with the smallest
    // degree. We count in the degree of v all vertices that are connected to v,
    // and belong to a different partition.
    struct InsertLowestDegreeVertexInSmallestColour : public RepairMove {
        InsertLowestDegreeVertexInSmallestColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertLowestDegreeVertexInSmallestColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };

    // Similar to InsertRandomVertexInRandomColour, but the vertex is chosen as
    // that, among the vertices of uncoloured partitions, with the smallest colour
    // degree. We count in the colour degree of v all vertices that are connected
    // to v, belong to a different partition, and are already coloured.
    struct InsertLowestColourDegreeVertexInRandomColour : public RepairMove {
        InsertLowestColourDegreeVertexInRandomColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertLowestColourDegreeVertexInRandomColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };

    // Similar to InsertRandomVertexInBiggestColour, but the vertex is chosen as
    // that, among the vertices of uncoloured partitions, with the smallest colour
    // degree. We count in the colour degree of v all vertices that are connected
    // to v, belong to a different partition, and are already coloured.
    struct InsertLowestColourDegreeVertexInBiggestColour : public RepairMove {
        InsertLowestColourDegreeVertexInBiggestColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertLowestColourDegreeVertexInBiggestColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };

    // Similar to InsertRandomVertexInSmallestColour, but the vertex is chosen as
    // that, among the vertices of uncoloured partitions, with the smallest colour
    // degree. We count in the colour degree of v all vertices that are connected
    // to v, belong to a different partition, and are already coloured.
    struct InsertLowestColourDegreeVertexInSmallestColour : public RepairMove {
        InsertLowestColourDegreeVertexInSmallestColour(const Graph& g, std::mt19937& mt) : RepairMove{g, mt} {}
        ~InsertLowestColourDegreeVertexInSmallestColour() {}
        void operator()(ALNSColouring& c, TabuList& tl, uint32_t iteration_number) const;
    };
}

#endif