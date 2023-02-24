![Children colouring](colouring.jpg)
<sub><sub>Human solvers for another type of colouring problem... Picture by San Mateo County Libraries distributed under License Attribution-NonCommercial-ShareAlike 2.0 Generic (CC BY-NC-SA 2.0)</sub></sub>

# Selective Graph Colouring and Max-Weight Selective Graph Colouring

This repository contains the source code for:

1. Heuristic and exact methods to solve the **Selective Graph Colouring Problem** (SGCP). This is an extension of the classical Graph Colouring Problem, when the vertex set is partitioned in clusters, and we only need to colour one vertex for each cluster. This problem is also known as the Partition Colouring Problem.
2. Exact methods to solve the **Max-Weight Selective Graph Colouring Problem** (MWSGCP). This is an extension of the SGCP, when each cluster is associated with a weight; each colour gets the weight of the heaviest cluster it covers (hence the name *max*-weight) and the objective is to minimise the sum of the weights of the colours. This problem is also known as the Maximum Weight Partition Colouring Problem.

## Citation

The reference paper for the **Selective Graph Colouring Problem** is the following:

```bib
@article{furini2018branch,
  title={An Exact Algorithm for the {Partition Coloring Problem}},
  author={Furini, Fabio and Malaguti, Enrico and Santini, Alberto},
  journal={{Computers \& Operations Research}},
  pages={170--181},
  volume=92,
  year=2018,
  doi={10.1016/j.cor.2017.12.019}
}
```

The reference paper for the **Maximum-Weight Selective Colouring Problem** is the following:

```bib
@article{cornaz2019selective,
  title={A note on selective line-graphs and partition colorings},
  author={Cornaz, Denis and Furini, Fabio and Malaguti, Enrico and Santini, Alberto},
  journal={{Operations Research Letters}},
  year=2019,
  volume=47,
  issue=6,
  pages={565--568},
  doi={10.1016/j.orl.2019.08.005}
}
```

You can also cite this repository through Zenodo.

[![DOI](https://zenodo.org/badge/91941968.svg)](https://zenodo.org/badge/latestdoi/91941968)

```bib
@misc{selective_colouring_github,
    title={Solvers for the Selective Graph Colouring Problem and generalisations},
    author={Santini, Alberto},
    date={2017-10-27},
    howpublished={Github repository},
    doi={10.5281/zenodo.91941968},
    url={https://github.com/alberto-santini/selective-graph-colouring/}
}
```

## Building the solvers

The easiest way to build is probably using cmake.
Prerequisites are [Boost](https://www.boost.org/) (for `base-sgcp`), [exactcolors](https://github.com/heldstephan/exactcolors) and IBM's cplex (for both `base-sgcp` and `max-weight-sgcp`), [ProgramOptions.hxx](https://github.com/Fytch/ProgramOptions.hxx) (for `max-weight-sgcp`).
Cmake config files are provided for exactcolors and cplex in folder `cmake` (boost is supported in cmake natively, so no additional config file is needed; ProgramOptions is header-only).
The main cmake configuration file is `CMakeLists.txt`.

For example, you can build both solvers with:

1.  Create a build directory: `mkdir build` and move to it `cd build`.
2.  Run cmake, e.g.: `cmake -DCMAKE_BUILD_TYPE=Release ..`.
3.  Run make, e.g.: `make -j4`.

Once make is done, you will have two executables: `sgcp` for the SGCP, and `mwsgcp` for the MWSGCP.
Below are some examples on how to run them.

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

You file detailed results in CSV format at the path you specified in the parameter file.

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

    ./mwsgcp -y weighted-stable-set -o solution.txt -g ../max-weight-sgcp/instances/wg-40-0.9-0.1.txt
    Reading graph from file...
    Graph read from file (0.00807633 s)
    Preparing Max-Weight Stable Set graph...
    Max-Weight Stable Set graph ready (0.0258555 s) 73 vertices and 1015 edges
    Launching the Max-Weight Stable Set solver (Sewell)...
    Greedy algorithm found  best_z 213.
    Sewell Stable Set solver finished (0.000491621 s)
    Stable Set solver result: 230 (465 - 235)

You will find detailed results in CSV format in the file specified with the `-o` switch.

## File structure (SGCP)

Files and folders are organised as follows.

### Folder `source-code`

This folder contains the C++ source files.
The various solution algorithms are:

* A branch-and-price algorithm (subfolder `branch-and-price`), used in the above paper
* A representative-based MIP formulation (subfolder `campelo-mip`)
* A compact MIP formulation (subfolder `compact-mip`)
* A decomposition method (subfolder `decomposition`)
* Various heuristics (subfolder `heuristics`):
  * Greedy heuristics, used in the above paper
  * A Tabu Search algorithm
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

## File structure (MWSGCP)

### Folder `max-weight-sgcp/source-code`

This folder contains the C++ source files.
Files `graph.{h,cpp}` implement an algorithm based on the Line Graph, for the SGCP.
This means that, indeed, you can use this solver for the base version of the Selective Graph Colouring Problem; however, the branch-and-price algorithm described above (and implemented in folder `base-sgcp`) should be your first choice, as it is more sophisticated.
Files `graph_weighted.{h,cpp}` implement the corresponding extension for the MWSGCP.

### Folder `max-weight-sgcp/instances`

This folder contains instances for the MWSGCP generated by me.
The instances have the same format as the SGCP instances, except that before the lines representing edges, there are `p` lines indicating the weight of each cluster.

## License

This source code in this repository is released under the GPLv3.0 license, as detailed in the file `LICENSE.txt`
