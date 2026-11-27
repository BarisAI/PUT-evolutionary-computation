# Assignment 8 - Test notes

The following checks were executed in a Linux container with g++/CMake and Python. These are actual automated tests, not claimed manual tests on a physical student's laptop. The instructions below work similarly on a normal C++17 laptop.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/assignment8 '../Assignment 1/data' results/local_optima.csv 1000
./build/assignment8_checker '../Assignment 1/data' results/local_optima.csv
python3 -m unittest discover -s scripts -p 'test_*.py' -v
python3 scripts/analyze.py
```

Results observed:

- CMake Release compilation: passed.
- C++ small-instance unit tests: passed.
- Greedy local optima: 1,000 for TSPA, 1,000 for TSPB.
- Standalone checker: **2,000/2,000 tours valid**, reported objectives correct, no improving 2-opt or replacement move remains.
- Python similarity tests: 5 passed.
- Independent Python full objective evaluation and previous ILS reference verification: passed.
- Average-overlap fast formula vs explicit all-other set intersections for selected samples: passed.
- Scatterplots: 12 SVGs; 999 points for best-of-sample plots, 1,000 for each other plot.

The generator uses fixed seeds but CPU time measurements may differ on other machines. C++ tests use small problems while the independent checker verifies every saved 100-node local optimum from both full instances.

Additional checks completed:

- AddressSanitizer + UndefinedBehaviorSanitizer build of the C++ small-instance unit tests: passed.
- Repeated the first 10 starts per instance using the same seeds. Objective, tour, accepted-move count and delta-check count matched the first 10 entries of the 1,000-run results for both TSPA and TSPB (timings are not expected to match).
- Negative test: changed one reported objective deliberately; independent checker rejected it.
- PDF visual inspection: 8 A4 pages; 12 scatterplots present and readable.
