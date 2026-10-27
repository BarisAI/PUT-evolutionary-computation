# Assignment 4 - Candidate Moves in Local Search

Students: Baris Surmelioglu - 152048; Ozan Polat - 150918

This assignment extends the random-start Steepest 2-opt + selected/unselected-replacement search from Assignment 3. It builds directed candidate-neighbour lists for each node ranked by D[u,v] + cost[v], and enumerates only moves that add at least one candidate edge. Two possible cut-pairs or replacement positions are generated for each candidate edge, and duplicates are removed before delta evaluation.

The primary K is 10; we also compare K=5 and K=20 with unrestricted Steepest. The full method and all candidate variants use exactly the same random starts.

## Build and reproduce

From inside the `Assignment 4` directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/assignment4 "../Assignment 1/data" results 200
./build/checker "../Assignment 1/data" results/best_solutions.txt
```

On Windows, run `.exe` binaries, possibly under `build/Release`. The third positional argument is the number of starting solutions per method/instance (1..200); 200 is the required full experiment.

## File contents

- `src/candidate_search.hpp` - candidate neighbour lists, both types of candidate moves, 2-opt/replacement delta formulas, unrestricted/candidate Steepest.
- `src/main.cpp` - 200 matched random starts for every method/instance, running times, statistics and 8 SVG graphics.
- `src/tests.cpp` - eligibility, both candidate-edge orientations, no duplicate moves, delta re-evaluation, K=n-1 completeness and local optimum checks.
- `src/checker.cpp` - independent checker using original CSV coordinates.
- `results/runs.csv` - all 1,600 run-level objective/timing/delta evaluation records.
- `results/summary.csv` - means/minima/maxima and time comparisons.
- `results/best_solutions.txt` - zero-based indices of eight best tours.
- `results/plots/` - eight 2D SVG best tours with node-cost coloring.
- `results/checker_output.txt` - independent checker pass log.
- `results/instructor_formula_check.csv` - verification according to Excel I2/J2/K2 computation rules.
- `REPORT.md` - detailed explanation, **complete pseudocode**, results, plots, tests and conclusions.

The previous Assignment 1/2/3 folders are not modified. We reuse `../Assignment 1/src/problem.hpp` to keep data parsing, rounding and objective definition unchanged.

### Timing note

`mean_local_ms` is only the search execution, measured with `std::chrono::steady_clock`. The `mean_total_ms` column adds one-time candidate-list preprocessing amortized over 200 starting solutions. Time spent loading CSV files, writing plots/reports and generating the shared random starts is excluded from both comparisons. Timing can vary on other CPUs.
