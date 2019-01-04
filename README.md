# Problems Considered

This repository contains the source code for:

1.  Heuristic and exact methods to solve the Selective Graph Colouring Problem (SGCP).
2.  Exact methods to solve the Min-Weight Selective Graph Colouring Problem (MWSGCP).

# Building the solvers

The easiest way to build is probably using cmake.
Prerequisites are [Boost](https://www.boost.org/) (for `base-sgcp`), [exactcolors](https://github.com/heldstephan/exactcolors) and IBM's cplex (for both `base-sgcp` and `min-weight-sgcp`), [ProgramOptions.hxx](https://github.com/Fytch/ProgramOptions.hxx) (for `min-weight-sgcp`).
Cmake config files are provided for exactcolors and cplex in folder `cmake` (boost is supported in cmake natively, so no additional config file is needed).
The main cmake configuration file is `CMakeLists.txt`.

For example, you can build both solvers with:

1.  Create a build directory: `mkdir build` and move to it `cd build`.
2.  Run cmake, e.g.: `cmake -DCMAKE_BUILD_TYPE=Release ..`.
3.  Run make, e.g.: `make -j4`.

Once make is done, you will have two executables: `sgcp` for the SGCP, and `mwsgcp` for the MWSGCP.
Above, some example on how to run them.

## Running `sgcp`

    ./sgcp ../base-sgcp/example-params.json ../base-sgcp/instances/random/n20p5t2s1.txt bp
    Reading graph in ../base-sgcp/instances/random/n20p5t2s1.txt
        20 vertices
        98 edges
        10 partitions
    Preprocessing removed 0 partitions.
    Preprocessing removed 0 additional vertices.
    Obtaining initial solution with greedy heuristic
    Applying Hoshino's populate method
    Hoshino's populate method generated 18 new columns starting from 13 existing ones.

    Starting branch-and-price algorithm!


    Node ID   LB        UB        Pool size     Open nodes
    *---------*---------*---------*-------------*---------
    Nodes in tree: 1
    Columns in global pool: 32

    Node id: 0, depth: 0

    LP Master Problem Solution: 3
        New column discarded: { 19 6 0 10 } (reduced cost: 1)

    MIP Master Problem Solution: 3
    Node solved to optimality.

    1         3         3         32            0

    ====================
    BB Tree exploration completed!
    Lower bound: 3 (=> 3)
    Upper bound: 3

    === Solution ===
    Colouring:
    0: { 10 6 5 } (valid? true)
    1: { 17 7 19 14 } (valid? true)
    2: { 4 0 15 } (valid? true)

## Running `mwsgcp`

    ./mwsgcp --help
    Usage:
    mwsgcp [options]
    Available options:
    -h, --help           Prints this help text.
    -y, --problem-type   Type of problem to solve. It can be either weighted-cli-
                            que, weighted-mip, weighted-stable-set or unweighted
                            (default).
    -t, --cplex-timeout  Timeout to pass to the CPLEX solver.
    -g, --graph          File containing the graph. Mandatory.
    -o, --output         File where we append the results. Mandatory.

    ./mwsgcp -y weighted-stable-set -o solution.txt -g ../min-weight-sgcp/instances/wg-40-0.9-0.1.txt
    Reading graph from file...
    Graph read from file (0.00807633 s)
    Preparing Max-Weight Stable Set graph...
    Max-Weight Stable Set graph ready (0.0258555 s) 73 vertices and 1015 edges
    Launching the Max-Weight Stable Set solver (Sewell)...
    Greedy algorithm found  best_z 213.
    Sewell Stable Set solver finished (0.000491621 s)
    Stable Set solver result: 230 (465 - 235)

# Selective Graph Colouring Problem

Relevant files for the SGCP is in folder `base-sgcp`.
The reference paper is:
    
    Fabio Furini, Enrico Malaguti, and Alberto Santini. An exact algorithm for the Partition Colouring Problem. Computers & Operations Resarch, 92:170-181, 2018.

Please cite this paper if you are using our solver in your research.

## File structure

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

### File `example-params.json`

This file can be used as a base for writing your own parameters file.
Unfortunately the original file used in the paper has been lost when the cluster at the University of Bologna broke down and no data was saved.
Using the information in the paper, however, it should be possible to reconstruct a config file which matches the original one exactly.

# Min-Weight Selective Colouring Problem

Code for the MWSGCP is in folder `min-weight-sgcp`.
The reference paper for these algorithms is yet a draft.

## File structure

### Folder `min-weight-sgcp/source-code`

This folder contains the C++ source files.
Files `graph.{h,cpp}` implement an algorithm based on the Maximum-Weight Stable Set problem, for the SGCP.
Files `graph_weighted.{h,cpp}` implement the corresponding extension for the MWSGCP.

### Folder `min-weight-sgcp/instances`

This folder contains instances for the MWSGCP generated by me.
The instances have the same format as the SGCP instances, except that before the lines representing edges, there are `p` lines indicating the weights of each cluster.

# License

This source code in this repository is released under the GPLv3.0 license, as detailed in the file `LICENSE.txt`
