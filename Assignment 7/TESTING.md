# Assignment 7 - Execution and Test Log

**Students:** Baris Surmelioglu (152048), Ozan Polat (150918)  
**Status:** actual commands were run in the assistant's Linux execution environment on 2026-11-20, not on a student's physical laptop. The commands below are provided for direct laptop reproduction; no manual hardware test is fabricated.

## Actual execution environment

- OS: Linux x86_64 (kernel 6.18.57), containerized execution environment.
- Compiler: g++ 14.2.0, C++17 release with `-O3 -Wall -Wextra -Wpedantic`.
- 5 logical CPUs reported; four local C++ worker processes ran simultaneous independently seeded experiments, interleaved in a fixed randomized job order.
- TSPA/TSPB read from the original `Assignment 1/data` directory; 200 nodes and 100 selected nodes each.
- Time budget: exact previous Assignment 6 measured mean MSLS wall-clock time, not a new benchmark estimated for this computer.
- **240 actual full-duration experiments**: six methods x 20 independent trials x two datasets.

## 1. Build, unit tests, property tests

From the `Assignment 7` directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Observed output:

```text
Test project .../Assignment 7/build
    Start 1: assignment7_property_tests
1/1 Test #1: assignment7_property_tests .......   Passed
100% tests passed, 0 tests failed out of 1
```

`src/tests.cpp` explicitly tested:

- Thirty percent of 100 selected nodes removes exactly 30 distinct nodes.
- Single circular subpath remains contiguous including wraparound.
- Multiple-subpath removal produces exactly 3 to 5 circular separated nonadjacent subpaths, with total size exactly 30 and retained gaps.
- Both weighted 2-regret repair bases maintain the original survivors and restore exactly 100 unique selected vertices with valid objective calculations.
- Multiple randomized synthetic problem sizes and seeds; invalid destroy arguments rejected.
- With-LS solutions have no improving move in the complete 2-opt + selected/unselected replacement neighborhood.
- Every method counts main-loop iterations, accepted/rejected proposals and Local Search calls correctly; the best-so-far objective cannot worsen.

## 2. AddressSanitizer and UndefinedBehaviorSanitizer

```sh
g++ -std=c++17 -O1 -g -Wall -Wextra -Wpedantic \
    -fno-omit-frame-pointer -fsanitize=address,undefined \
    src/tests.cpp -o assignment7_tests_asan
ASAN_OPTIONS=detect_leaks=1 ./assignment7_tests_asan
```

Observed: `PASS: 30% exact; circular subpaths; 3-5 distinct subpaths; weighted regret repair both bases; feasibility; optimum; duration; accounting; random property cases.` No sanitizer errors were reported in this test.

## 3. Real terminal smoke test

```sh
./build/assignment7 '../Assignment 1/data' TSPA 4 0 120
```

The first 15 CSV fields observed (followed by the full valid 100-node tour):

```text
TSPA,multiple_subpaths,without_ls,0,2048723,120.000000,120.207829,74161,71792,154,19,135,1,2093850,weighted_cycle
```

Field meanings: instance, destroy, post-repair LS mode, trial, seed, target milliseconds, observed milliseconds, initial objective, best objective, main-loop count, accepted, rejected, LS calls, delta evaluations, repair.

## 4. Full 240-trial experiment

```sh
python3 scripts/run_experiments.py --workers 4
```

Observed final lines:

```text
COMPLETED 240 / 240 ...
Saved 240 rows and 12 summary groups
```

All 240 experiments were real C++ binary executions with measured elapsed time, not generated, estimated or simulated table values. The randomized mixed schedule reduces systematic method-order effects. CPU contention is possible; `--workers 1` reproduces serial execution on a personal laptop, though the exact iteration counts are expected to vary with processor and load. The exact per-trial values are saved in `results/runs.csv` and the host/compiler summary in `results/run_info.json`.

## 5. Independent Python audit of every run

```sh
python3 scripts/audit_results.py
```

Observed:

```text
PASS: 240/240 independently re-evaluated with rounded closed edges and selected node costs.
PASS: 12/12 groups have 20 real trials, summary statistics and loop counts verified.
PASS: 120/120 paired trials have the same initial solution objective and seed.
PASS: 12/12 saved best tours match run-level winners and all measured time budgets.
```

The auditor reads *original CSV coordinates*, computes rounded Euclidean distances and vertex costs independently, checks every full cycle, independently recomputes means/minima/maxima, and checks each time limit and each paired seed.

## 6. Independent C++ checker of all 12 best tours

```sh
./build/assignment7_checker '../Assignment 1/data' results/best_solutions.txt
```

Observed: all 12 saved best tours passed. Each closed tour has exactly 100 unique valid zero-based nodes and the expected total cost. For every best **with-LS** tour, the checker also enumerated the full 2-opt + replacement neighborhood and verified no negative delta remained. Full per-method checks are in `results/checker_output.txt`.

## 7. Figures and report consistency

```sh
python3 scripts/make_plots.py
python3 scripts/assemble_report.py
python3 scripts/build_pdf.py
```

- Twelve best-tour SVGs and two experiment-comparison SVGs produced from saved actual results.
- Each graph's displayed travel length plus selected-node cost is independently checked against the reported objective.
- The PDF report was rendered and inspected; it uses the same final `REPORT.md` and data as the GitHub files.

## Reproduction checklist for a student laptop

1. Clone the repo, open `Assignment 7`, and build with CMake.
2. Run the actual unit tests with `ctest` and verify the PASS output.
3. Run the short terminal smoke test and inspect the returned valid tour and iteration counters.
4. Run `python3 scripts/run_experiments.py --workers 1` for the full serial experiment. This takes longer than a smoke test; avoid other CPU-intensive work while measuring.
5. Run `assignment7_checker`, `audit_results.py`, `make_plots.py`, and compare the new `summary.csv` with the versioned report. The *objectives* can vary if timing changes the number of iterations, while all correctness requirements must hold.
