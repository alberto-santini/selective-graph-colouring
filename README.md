## Selective Graph Colouring Problem

This repository contains the source code for heuristic and exact methods to solve the Selective Graph Colouring Problem.
The reference paper for these algorithms is:

    Fabio Furini, Enrico Malaguti, and Alberto Santini. An exact algorithm for the Partition Colouring Problem. Computers & Operations Resarch (under revision), 2017


### Folder `source-code`

This folder contains the C++ source files.
The various solution algorithms are:

* A branch-and-price algorithm (subfolder `branch-and-price`), used in the above paper
* A representative-based MIP (subfolder `campelo-mip`)
* A compact MIP formulation (subfolder `compact-mip`)
* A decomposition method (subfolder `decomposition`)
* Various heuristics (subfolder `heuristics`):
  * Greedy heuristics, used in the above paper
  * A Tabu Search algorithm, used in the above paper
  * An Adaptive Large Neighbourhood Search algorithm, used in the above paper
  * A Greedy Randomised Search Procedure

### Folder `instances`

This folder contains test instances from the literature.
There are four test sets: `random`, `big-random`, `nsfnet`, and `ring`.
The instance format is the following:

* The first line only has one number `n`, which is the number of vertices in the graph (numbered from `0` to `n-1`)
* The second line only has one number `m`, which is the number of edges in the graph
* The third line only has one number `p`, which is the number of clusters in the partition
* The next `m` lines have two numbers, and represent the edges
* The next `p` lines have at least 1 number, and represent the clusters

### License

This source code in this repository is released under the GPLv3.0 license, as detailed in the file `LICENSE.txt`
