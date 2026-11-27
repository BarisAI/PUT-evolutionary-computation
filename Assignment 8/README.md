# Assignment 8 - Global convexity tests

Baris Surmelioglu (152048) and Ozan Polat (150918)

We generate 1,000 random-start Greedy Local Search local optima for each of the original TSPA and TSPB instances. Then we compare objective values with overlap in selected nodes or undirected tour edges.

## Build and run

From `Assignment 8` in the repository:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/assignment8 '../Assignment 1/data' results/local_optima.csv 1000
./build/assignment8_checker '../Assignment 1/data' results/local_optima.csv
python3 -m unittest discover -s scripts -p 'test_*.py' -v
python3 scripts/analyze.py
```

On Windows, executables can be in `build/Release` and have `.exe` extensions. C++17 needs only the standard library. The analysis uses Python 3, NumPy, and Matplotlib. The experiment runner is deterministic given the input data and its seeds, while measured runtimes vary by computer.

## Files

- `src/main.cpp`: 1,000 Greedy Local Search runs per instance. Uses the existing random-tour and Greedy Local Search implementation (randomized first improvement, 2-opt and selected/unselected replacement).
- `src/checker.cpp`: independently checks every final objective and whether any improving 2-opt or replacement remains.
- `src/tests.cpp`: C++ unit tests on small instances.
- `scripts/analyze.py`: objective checks, two overlap measures, self-exclusion, 12 Pearson correlations and plots.
- `scripts/test_analysis.py`: Python unit tests, including edge direction and self-exclusion.
- `results/local_optima.csv`: 2,000 local optima with complete 100-node tours.
- `results/metrics.csv`: all 12 Pearson coefficients.
- `results/points.csv`: original (objective, similarity) values for every plot.
- `results/optima_summary.csv`: sample objective ranges and counts.
- `results/references.txt`: exact two reference tours per instance and their provenance.
- `results/charts/`: twelve scatterplots in SVG.
- `REPORT.md`: methods, pseudocode, results and comments.
- `TESTING.md`: commands and observed checks.

The previous-method reference for each instance is the best ILS tour saved in `../Assignment 6/results/best_solutions.txt`. It is independent of the 1,000 sampled Greedy local optima. Every sample-best comparison excludes that sample run itself.
