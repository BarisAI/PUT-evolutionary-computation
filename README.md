# PUT Evolutionary Computation

Course assignments implemented in C++.

## Assignment 1 — Greedy heuristics

- [Report, pseudocode, results, best tours and conclusions](Assignment%201/REPORT.md)
- [Build and reproduction instructions](Assignment%201/README.md)
- [C++ source code](Assignment%201/src)

## Assignment 2 — Greedy regret heuristics

- [Report, pseudocode, previous/new results, best tours and conclusions](Assignment%202/REPORT.md)
- [Build and reproduction instructions](Assignment%202/README.md)
- [C++ source code](Assignment%202/src)

The second assignment adds 2-regret and weighted 2-regret constructions based on extended nearest neighbor and greedy cycle, with the same TSPA/TSPB problem and checker. Assignment 1 remains unchanged.

The first assignment implements random solutions, nearest neighbor at the end, nearest neighbor at all positions, and greedy cycle for the weighted partial traveling salesperson problem.

## Assignment 3 - Local Search

- [Report, all-method objective/runtime comparisons and 16 best-tour figures](Assignment%203/REPORT.md)
- [Build and reproduction instructions](Assignment%203/README.md)
- [Steepest and basic randomized first-improvement Greedy implementation](Assignment%203/src)

This assignment adds 8 local-search combinations using node swaps or 2-opt, selected/unselected replacement and random or constructive starting tours. Previous assignments are unchanged.

## Assignment 4 - Candidate Moves in Local Search

- [Detailed report with candidate enumeration pseudocode, validation and visualizations](Assignment%204/REPORT.md)
- [Build and reproduction instructions](Assignment%204/README.md)
- [Candidate-edge Steepest source and tests](Assignment%204/src)

Steepest 2-opt + inter-route node replacement with random starting tours. A full-neighborhood baseline is compared against candidate-only neighborhoods with K=5, K=10 and K=20, using the same 200 starts on TSPA and TSPB. Previous assignments are preserved.

## Assignment 5 - Reusing Move Evaluations in Local Search

- [Detailed report, pseudocode, experiments and four best-tour graphs](Assignment%205/REPORT.md)
- [Reproduction instructions](Assignment%205/README.md)
- [C++17 LM implementation and unit tests](Assignment%205/src)

Assignment 5 reuses the unrestricted 2-opt + node-replacement steepest neighborhood from Assignment 3. A saved improving-move queue retains previous deltas and handles missing cut edges, relative edge reversals and applicable moves. Both methods are compared on the same 200 random starting tours for TSPA and TSPB. Assignment 1-4 folders remain unchanged.

## Assignment 6 - Multiple Start Local Search (MSLS) and Iterated Local Search (ILS)

- [Detailed report with MSLS/ILS pseudocode, perturbation, results, and best tours](Assignment%206/REPORT.md)
- [Build and reproduction instructions](Assignment%206/README.md)
- [C++17 implementations and tests](Assignment%206/src)

MSLS runs 400 independent random-start full Steepest local searches per trial; ILS uses random-start local search with four selected/unselected exchanges, a double-bridge-style tour perturbation, and time budget equal to the observed average MSLS runtime. Both methods were tested 20 times for each of TSPA and TSPB. Earlier assignment folders are unchanged.
