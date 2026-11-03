# Assignment 5 - List of Improving Moves (C++17)

**Students:** Baris Surmelioglu - 152048; Ozan Polat - 150918  
**Course:** Evolutionary Computation

This assignment implements Steepest Local Search with **cached improving deltas from prior iterations (LM)**. It uses the unrestricted **2-opt + selected/unselected replacement** neighborhood from Assignment 3, not candidate moves. The full Steepest baseline is rerun using exactly the same 200 random starting tours per instance. TSPA/TSPB data and the original full-neighborhood algorithm are reused unchanged from earlier assignments.

## Implementation

- `src/lm_search.hpp`: move identity by removed-edge endpoints, both 2-opt orientations, saved objective deltas, and a priority-queue LM.
- Removed edge no longer present -> discard saved move.
- Exactly one edge relatively reversed -> defer until another iteration.
- Same relative direction (even if both edges are reversed) -> apply stored delta.
- Selected/unselected replacement checks saved neighbors and selection status.
- After a move, only newly defined deltas (new edges, changed node neighbors, newly excluded node) are evaluated. One full refresh on exhaustion verifies a true full-neighborhood local optimum.
- The heap may contain a small number of duplicate descriptors; lazy validity checks safely remove stale copies. No candidate-move filtering is used.

## Build and reproduce

From repository root on Linux/macOS (or equivalent Windows paths):

```sh
cmake -S "Assignment 5" -B "Assignment 5/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "Assignment 5/build" --config Release
ctest --test-dir "Assignment 5/build" -C Release --output-on-failure
cd "Assignment 5"
./build/assignment5 "../Assignment 1/data" results 200
./build/checker "../Assignment 1/data" results/best_solutions.txt
```

On Windows with multi-configuration CMake, executable files may be in `build/Release/` and use the `.exe` extension. All output is generated in `results/` and existing result files are overwritten when reproducing.

## Experiment and files

- 2 methods (full Steepest baseline and LM) x 200 random starts x 2 instances = **800 runs**.
- Seed 42, TSPA starts generated before TSPB, identical starting tours for both methods.
- Running times are search-only in ms, measured using `steady_clock`. They exclude shared random-start generation, file writes and SVG output; results vary across machines.
- `results/runs.csv`: all 800 runs, objectives, delta checks, accepted move counts and wall-clock times.
- `results/summary.csv`: min/max/average objective, average runtime and counts.
- `results/best_solutions.txt`: 4 best tours, zero-based indices.
- `results/plots/`: 4 SVG graphs.
- `results/checker_output.txt`: independent checker output.
- `results/instructor_formula_check.csv`: programmatic Excel-rule verification.
- `REPORT.md`: full pseudocode, technical explanation, results, graphs, verification, conclusions.

All previous assignment folders remain unchanged.
