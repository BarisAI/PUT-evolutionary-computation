# Assignment 6 - Multiple Start (MSLS) and Iterated Local Search (ILS)

Students: **Baris Surmelioglu - 152048** and **Ozan Polat - 150918**  
Course: Evolutionary Computation

The project reuses the original CSV data and exact full 2-opt + selected/unselected replacement Steepest Local Search from Assignments 1 and 3. Previous assignments remain unchanged.

## Build and run

From the `Assignment 6` directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/assignment6 '../Assignment 1/data' results 20 400
./build/checker '../Assignment 1/data' results/best_solutions.txt
```

On Windows, executables can be in `build/Release` and end with `.exe`.

The program takes `data_dir`, `output_dir`, optional `repeats` (default **20**) and optional `msls_restarts` (default **400**). **Only the default 20 x 400 experiment is submitted as final**; smaller arguments are for smoke testing.

## Algorithms

* **MSLS:** each of 20 top-level runs executes 400 independent random-start Steepest searches, returning the minimum objective among those 400 local optima.
* **ILS:** each of 20 top-level runs starts from one fresh random tour. Perturb four selected nodes via unique selected/unselected replacements and perform one double-bridge segment reorder, then run Steepest. Accept nonworsening candidates, otherwise accept with probability 0.12; restore best current state after 80 unsuccessful kicks. Save overall best independently. Stop after the *mean measured MSLS duration on the same instance*.

## Submitted results

- `REPORT.md`: detailed pseudocode, parameter choices, comparisons and conclusions
- `results/runs.csv`: all 80 top-level experiments and LS call counts
- `results/summary.csv`: averages, minima/maxima, mean running times and mean LS calls
- `results/budgets.csv`: time budgets actually used for ILS
- `results/progress.csv`: improvements of best-so-far objective
- `results/best_solutions.txt`: four best 100-node tours (0-based)
- `results/checker_output.txt`: direct coordinate checker, closure and local-optimum checks
- `results/instructor_formula_check.csv`: programmatic I2/J2/K2 calculations
- `results/plots/`: four SVG route plots (one best tour per instance and method)
- `src/tests.cpp`: perturbation, feasibility, local-optimum, time budget and accounting tests

Run times depend on hardware and system load; experiment RNG seeds and the exact perturbation mechanism are documented in `REPORT.md` and `results/run_info.txt`.
