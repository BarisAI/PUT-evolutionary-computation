# Assignment 2 — Greedy regret heuristics (C++17)

**Students:** Baris Surmelioglu - 152048; Ozan Polat - 150918
**Course:** Evolutionary Computation

Four new variants are implemented, combining two criteria with two construction
methods:

- nn_2regret — pure 2-regret, all-position nearest-neighbor (open path)
- cycle_2regret — pure 2-regret, greedy closed cycle
- nn_weighted_2regret — weighted 2-regret, open path
- cycle_weighted_2regret — weighted 2-regret, closed cycle

For each unused node, regret equals the second-best insertion delta minus the
best one. Pure regret chooses maximum regret. Weighted regret chooses the
maximum of 0.5 * regret - 0.5 * best_delta. Every insertion delta includes node
cost. All positions are considered for every candidate; ties favor the smaller
best delta and then smaller node/position index.

Both TSPA and TSPB come unchanged from ../Assignment 1/data. The implementation
reuses ../Assignment 1/src/problem.hpp for its rounded distance matrix and
closed-cycle objective. Source code, tests, runs, statistics and 8 SVG figures
belong exclusively to this folder.

## Compile and run

From the Assignment 2 folder:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/assignment2 "../Assignment 1/data" results 0.5 0.5
./build/checker "../Assignment 1/data" results/best_solutions.txt
```

On Windows the executables may be inside build/Release and end in .exe.
The default weights are 0.5 and 0.5 and may be passed as last two parameters.

## Files

- REPORT.md — problem, pseudocode, results compared with Assignment 1,
  eight best routes/plots, checker information and conclusions.
- src/regret.hpp — 2-regret and weighted 2-regret, open or closed.
- src/main.cpp — 1,600 experiment runs and SVG visualization.
- src/checker.cpp — coordinate-based independent solution checker.
- src/tests.cpp — deterministic insertion, corner case and validation tests.
- results/runs.csv — every new run.
- results/summary.csv — new methods, minima/maxima/means.
- results/comparison_summary.csv — 4 previous and 4 new methods, both instances.
- results/best_solutions.txt — eight best 0-based node-index lists.
- results/checker_output.txt — independent checker output.
- results/instructor_checker_validation.csv — spreadsheet-rule validation.
- results/plots/ — eight 2D SVG graphics showing node cost.

No Assignment 1 files are modified.
