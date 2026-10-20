# Assignment 3 - Local Search (C++17)

**Students:** Baris Surmelioglu - 152048; Ozan Polat - 150918

This folder adds two versions of local search (Steepest and randomized first-improvement Greedy), with node-swap or 2-opt intra-route moves, and selected/unselected replacement inter-route moves. Starting solutions are either random or the best previously measured constructive heuristic per instance: weighted Greedy Cycle for TSPA and weighted Nearest Neighbor for TSPB.

Eight combinations, two instances, 200 starts per combination = 3,200 local searches. See `REPORT.md` for formulas, pseudocode, objective/runtime tables for ALL methods from Assignments 1-3, 16 SVG plots, best solution indices, checks and conclusions.

The C++ sources from previous assignments are included via relative paths and **not modified**. The data files `../Assignment 1/data/TSPA.csv` and `TSPB.csv` are reused as they are.

## Compile and run

From `Assignment 3/`:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
./build/assignment3 "../Assignment 1/data" results 200
./build/checker "../Assignment 1/data" results/best_solutions.txt
```

For Windows Visual Studio generators the executables may be under `build/Release/` with `.exe` extensions. The optional third program argument is the number of starts (1..200), useful for smoke tests. Full reporting always uses 200.

## Key points

- **Steepest:** examine the WHOLE combined neighborhood; choose globally most negative objective delta.
- **Basic Greedy:** uniformly randomized move ordering using incremental Fisher-Yates, computing each delta ONLY WHEN VISITED and stopping at the first improving move. Both kinds of moves share one randomized pool. No candidate moves, old-iteration delta caching or full neighbor scoring.
- **2-opt** reverses a segment; swaps and replacements update only affected edges and, when appropriate, selected-node cost.
- All methods terminate at a local optimum and final tours are independently verified.
- The random start tours are identical across random-start variants, and greedy-constructed start tours identical across constructed-start variants for controlled comparisons.
- Total runtimes in milliseconds include initial solution construction and local search. `runtime_summary.csv` also separately includes local-search-only averages. The older algorithms were re-executed and timed on the same system rather than copying old runtime estimates.
- Results can differ in runtime on another computer; objective values are reproducible given the same seed and compiler-independent algorithms.

## Files

| File | Description |
|---|---|
| `src/local_search.hpp` | Delta moves, basic randomized greedy, steepest search |
| `src/main.cpp` | Runner, performance timing, historic-method reruns, SVG plots |
| `src/tests.cpp` | Exact delta tests and convergence/reproducibility tests |
| `src/checker.cpp` | Independent objective and tour validity verification |
| `results/runs.csv` | All 3,200 local search runs and delta checks |
| `results/previous_runs.csv` | Reruns of all 3,200 earlier-method experiments with timings |
| `results/objective_summary.csv` | Objectives for all 16 algorithms on both instances |
| `results/runtime_summary.csv` | End-to-end times, search-only times for new methods |
| `results/comparison_objectives.csv` | Requested 2-instance objective comparison table |
| `results/comparison_runtime.csv` | Requested 2-instance runtime comparison table |
| `results/best_solutions.txt` | 16 best tours (100 distinct zero-based indices) |
| `results/instructor_excel_validation.csv` | Excel-checker formula cross-check, 16 PASS |
| `results/checker_output.txt` | Independent checker, 16 PASS |
| `results/plots/*.svg` | 16 2D cost-colored tours |
| `REPORT.md` | English student-style report |
