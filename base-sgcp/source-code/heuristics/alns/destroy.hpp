#ifndef _DESTROY_HPP
#define _DESTROY_HPP

#include "../../graph.hpp"
#include "alns_colouring.hpp"

#include <random>

namespace sgcp {
    // A destroy move takes a complete ALNS colouring and turns it into an
    // incomplete colouring, by uncolouring one or more vertices.
    struct DestroyMove {
        const Graph& g;
        std::mt19937& mt;

        DestroyMove(const Graph& g, std::mt19937& mt) : g{g}, mt{mt} {}
        virtual ~DestroyMove() {}

        virtual void operator()(ALNSColouring& c) const = 0;
    };

    // Takes a random colour i in [0, n_colours - 1] and removes a random
    // vertex from colours[i].
    struct RemoveRandomVertexInRandomColour : public DestroyMove {
        RemoveRandomVertexInRandomColour(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveRandomVertexInRandomColour() {}
        void operator()(ALNSColouring& c) const;
    };

    // Takes the colour i with the smallest number of vertices coloured by i
    // and removes a random vertex from colours[i].
    struct RemoveRandomVertexInSmallestColour : public DestroyMove {
        RemoveRandomVertexInSmallestColour(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveRandomVertexInSmallestColour() {}
        void operator()(ALNSColouring& c) const;
    };

    // Takes the colour i with the biggest number of vertices coloured by i
    // and removes a random vertex from colours[i].
    struct RemoveRandomVertexInBiggestColour : public DestroyMove {
        RemoveRandomVertexInBiggestColour(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveRandomVertexInBiggestColour() {}
        void operator()(ALNSColouring& c) const;
    };

    // Removes that vertex from coloured_vertices, which has the smallest degree.
    // We count in the degree of v all vertices that are connected to v, and
    // belong to a different partition.
    struct RemoveVertexWithSmallestDegree : public DestroyMove {
        RemoveVertexWithSmallestDegree(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveVertexWithSmallestDegree() {}
        void operator()(ALNSColouring& c) const;
    };

    // Removes that vertex from coloured_vertices, which has the biggest degree.
    // We count in the degree of v all vertices that are connected to v, and
    // belong to a different partition.
    struct RemoveVertexWithBiggestDegree : public DestroyMove {
        RemoveVertexWithBiggestDegree(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveVertexWithBiggestDegree() {}
        void operator()(ALNSColouring& c) const;
    };

    // Removes that vertex from coloured_vertices, which has the smallest colour
    // degree. We count in the colour degree of v all vertices that are connected
    // to v, belong to a different partition, and are already coloured.
    struct RemoveVertexWithSmallestColourDegree : public DestroyMove {
        RemoveVertexWithSmallestColourDegree(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveVertexWithSmallestColourDegree() {}
        void operator()(ALNSColouring& c) const;
    };

    // Removes that vertex from coloured_vertices, which has the smallest colour
    // degree. We count in the colour degree of v all vertices that are connected
    // to v, belong to a different partition, and are already coloured.
    struct RemoveVertexWithBiggestColourDegree : public DestroyMove {
        RemoveVertexWithBiggestColourDegree(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveVertexWithBiggestColourDegree() {}
        void operator()(ALNSColouring& c) const;
    };

    // Removes a vertex from coloured_vertices with a probability inversely
    // proportional to the vertex's degree. We count in the degree of v all
    // vertices that are connected to v, and belong to a different partition.
    struct RemoveVertexByRouletteDegreeSmall : public DestroyMove {
        RemoveVertexByRouletteDegreeSmall(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveVertexByRouletteDegreeSmall() {}
        void operator()(ALNSColouring& c) const;
    };

    // Removes a vertex from coloured_vertices with a probability directly
    // proportional to the vertex's degree. We count in the degree of v all
    // vertices that are connected to v, and belong to a different partition.
    struct RemoveVertexByRouletteDegreeBig : public DestroyMove {
        RemoveVertexByRouletteDegreeBig(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveVertexByRouletteDegreeBig() {}
        void operator()(ALNSColouring& c) const;
    };

    // Removes a vertex from coloured_vertices with a probability inversely
    // proportional to the vertex's colour degree. We count in the colour
    // degree of v all vertices that are connected to v, belong to a different
    // partition, and are already coloured.
    struct RemoveVertexByRouletteColourDegreeSmall : public DestroyMove {
        RemoveVertexByRouletteColourDegreeSmall(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveVertexByRouletteColourDegreeSmall() {}
        void operator()(ALNSColouring& c) const;
    };

    // Removes a vertex from coloured_vertices with a probability directly
    // proportional to the vertex's colour degree. We count in the colour
    // degree of v all vertices that are connected to v, belong to a different
    // partition, and are already coloured.
    struct RemoveVertexByRouletteColourDegreeBig : public DestroyMove {
        RemoveVertexByRouletteColourDegreeBig(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveVertexByRouletteColourDegreeBig() {}
        void operator()(ALNSColouring& c) const;
    };

    // Takes a random colour i in [0, n_colours] and remove all vertices
    // coloured with i.
    struct RemoveRandomColour : public DestroyMove {
        RemoveRandomColour(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveRandomColour() {}
        void operator()(ALNSColouring& c) const;
    };

    // Takes the colour with fewest vertices and remove all vertices coloured
    // with that colour.
    struct RemoveSmallestColour : public DestroyMove {
        RemoveSmallestColour(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveSmallestColour() {}
        void operator()(ALNSColouring& c) const;
    };

    // Remove all vertices coloured with colour i, where i is the colour with
    // the smallest degree. The degree of i is the sum of the degrees of vertices
    // coloured with i, calculated as in RemoveVertexWithSmallestDegree.
    struct RemoveColourWithSmallestDegree : public DestroyMove {
        RemoveColourWithSmallestDegree(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveColourWithSmallestDegree() {}
        void operator()(ALNSColouring& c) const;
    };

    // Remove all vertices coloured with colour i, where i is the colour with
    // the smallest colour degree. The colour degree of i is the sum of the colour
    // degrees of vertices coloured with i, calculated as in
    // RemoveVertexWithSmallestColourDegree.
    struct RemoveColourWithSmallestColourDegree : public DestroyMove {
        RemoveColourWithSmallestColourDegree(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveColourWithSmallestColourDegree() {}
        void operator()(ALNSColouring& c) const;
    };

    // Remove all vertices coloured with colour i, where i is chosen with probability
    // inversely proportional to its degree. The degree of i is the sum of the
    // degrees of vertices coloured with i, calculated as in RemoveVertexWithSmallestDegree.
    struct RemoveColourByRouletteDegreeSmall : public DestroyMove {
        RemoveColourByRouletteDegreeSmall(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveColourByRouletteDegreeSmall() {}
        void operator()(ALNSColouring& c) const;
    };

    // Remove all vertices coloured with colour i, where i is chosen with probability
    // inversely proportional to its colour degree. The colour degree of i is the sum
    // of the colour degrees of vertices coloured with i, calculated as in
    // RemoveVertexWithSmallestDegree.
    struct RemoveColourByRouletteColourDegreeSmall : public DestroyMove {
        RemoveColourByRouletteColourDegreeSmall(const Graph& g, std::mt19937& mt) : DestroyMove{g, mt} {}
        ~RemoveColourByRouletteColourDegreeSmall() {}
        void operator()(ALNSColouring& c) const;
    };
}

#endif
