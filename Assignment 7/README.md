# Assignment 7 - Large Neighborhood Search (C++17)

**Students:** Baris Surmelioglu - 152048; Ozan Polat - 150918  
**Course:** Evolutionary Computation, Poznan University of Technology

Six LNS variants: three independent destroy operators (30% removal of selected nodes) with and without post-repair local search. Two instances, 20 complete time-budgeted runs per method/instance.

## Build and test on a normal laptop

From `Assignment 7` in the repository:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows, executables may live in `build/Release` and have `.exe` extension. No third-party C++ libraries are required.

A quick manual smoke test (Linux/macOS):

```sh
./build/assignment7 '../Assignment 1/data' TSPA 0 0 150
```

This prints **one actual experimental run** as a CSV line, including the seed, objective, iteration count, and tour. Method IDs are 0-5: `random_nodes` without/with LS, `one_subpath` without/with LS, `multiple_subpaths` without/with LS.

## Full experiment (this takes several minutes)

```sh
python3 scripts/run_experiments.py --workers 1
./build/assignment7_checker '../Assignment 1/data' results/best_solutions.txt
python3 scripts/make_plots.py
```

The original submitted experiment used four parallel local C++ workers with a randomized balanced schedule (`--workers 4`). Use `--workers 1` for serial laptop reproducibility; timings and iteration counts depend on CPU load. All seeds, original run outputs and local environment information are retained.

The time budget is taken from the **measured MSLS mean** in `../Assignment 6/results/budgets.csv` on the same TSPA/TSPB instance. Our report and experiment files refer to the original Assignment 1 input files, not modified copies.

## Project files

- `src/lns.hpp`: three destroy operators, weighted regret repair, full steepest reuse, six LNS variants.
- `src/main.cpp`: real C++ single-trial CLI and counters.
- `src/tests.cpp`: randomized property, feasibility, local-optimum, and edge-case tests.
- `src/checker.cpp`: independent re-evaluation of 12 best solutions, including closed edges.
- `results/runs.csv`: all 240 actual time-budgeted experiments.
- `results/summary.csv`: objective, runtime, main-loop iterations and LS calls.
- `results/best_solutions.txt`: all 12 winning node sequences (zero-based).
- `results/plots/`: 12 best-tour SVGs plus 2 comparison SVGs; PNGs are for PDF report generation.
- `REPORT.md`: detailed methods and pseudocode, reference comparisons, complete results, testing and conclusions.
- `TESTING.md`: command transcripts and technical verification, with truthful execution-environment descriptions.

**Clarification:** The assignment slide displays `f(y) > f(x)`, but the original objective must be **minimized**. The implemented acceptance check is `f(y) < f(x)`; otherwise the search would favor strictly worse solutions.
