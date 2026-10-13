# Assignment 1 — Greedy heuristics (C++17)

Complete experiment for the two supplied 200-node instances. All algorithms,
validation, CSV export and SVG plotting are implemented in C++17 using only the
standard library. No external packages are needed.

## Build and run with g++

Run these commands from this directory (`Assignment 1`):

```sh
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/main.cpp -o build/assignment1
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/checker.cpp -o build/checker
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/tests.cpp -o build/heuristic_tests
./build/heuristic_tests
./build/assignment1 data results 42
./build/checker data results/best_solutions.txt
```

On Windows, add `.exe` to executable filenames when required. A compiler with
C++17 filesystem support is necessary (for example GCC 9 or newer).

## Alternative CMake build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/assignment1 data results 42
./build/checker data results/best_solutions.txt
```

For multi-configuration generators such as Visual Studio, executables are under
`build/Release/`. The submitted code was compiled and tested directly with g++;
CMake was unavailable in the execution environment.

## Files

| Path | Content |
| --- | --- |
| `REPORT.md` | Full English report, pseudocode, results, figures, best node lists, conclusions |
| `src/problem.hpp` | CSV loading, rounded distance matrix, objective and validation |
| `src/heuristics.hpp` | Four construction methods, matrix-only optimization |
| `src/main.cpp` | Reproducible experiment, CSV/tour export, SVG visualization |
| `src/checker.cpp` | Independent checker using original coordinates |
| `src/tests.cpp` | Rounding, objective, insertion deltas, costs, edge cases and reproducibility tests |
| `data/TSPA.csv`, `data/TSPB.csv` | Original supplied data; rows define zero-based indices |
| `results/summary.csv` | Minimum, maximum and average for all instance/method pairs |
| `results/runs.csv` | All 1,600 measured objectives and starting nodes |
| `results/best_solutions.txt` | Eight best tours with claimed objectives |
| `results/checker_output.txt` | Independent C++ checker results |
| `results/instructor_checker_validation.csv` | Eight best tours checked with the instructor's Excel calculation rules |
| `results/plots/*.svg` | Eight best-tour visualizations with node-cost colors |

The runner accepts `DATA_DIRECTORY OUTPUT_DIRECTORY SEED` (default: `data results
42`). It runs 200 random constructions per instance and one greedy construction
per starting node (200 for each supplied instance). Random run start is written
as `-1`; greedy starts are 0–199. The random generator is initialized once and
consumed in TSPA then TSPB order. Strict node/position order resolves greedy ties;
the first minimum is retained in the results. The cycle closes implicitly from
the last index to the first; do not repeat the first index in a tour file.

**Interpretation:** nearest-neighbor methods grow an open path and include node
costs in every insertion decision; the completed path is then closed and scored
as a cycle. Greedy cycle includes the closing edge during construction. This
distinction and all formulas are documented in the report. The slide says
"three methods" but lists four; all four listed methods are implemented.

All eight solutions passed the included independent C++ checker and were also
validated against the instructor-provided `Solution checker.xlsx` calculation
formulas (100-node lists inserted into F3:F102, with I2/J2/K2 recalculated).
The Excel workbook's shared-formula groups were expanded in memory to preserve
its formulas during programmatic import; the original workbook was not modified.
See [REPORT.md](REPORT.md#5-solution-checking-and-tests) and
[results/instructor_checker_validation.csv](results/instructor_checker_validation.csv)
for the detailed method and eight independently recorded Excel-rule outcomes.
