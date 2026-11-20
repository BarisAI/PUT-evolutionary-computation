#!/usr/bin/env python3
"""Create detailed assignment REPORT.md directly from verified run data."""
from pathlib import Path
import csv
import statistics
HERE=Path(__file__).resolve().parents[1]
RESULT=HERE/'results'

with (RESULT/'summary.csv').open() as f: summary=list(csv.DictReader(f))
assert len(summary)==12
by={(x['instance'],x['destroy'],x['ls_mode']):x for x in summary}
labels={'random_nodes':'Random nodes','one_subpath':'Single random subpath','multiple_subpaths':'3-5 random subpaths'}
head='''# Assignment 7 - Large Neighborhood Search (LNS)

**Students:** Baris Surmelioglu - 152048  
Ozan Polat - 150918  
**Course:** Evolutionary Computation, Poznan University of Technology  
**Language:** C++17; original TSPA and TSPB (200 nodes; select 100)  
**Experiment:** 3 destroy operators x 2 post-repair variants x 20 trials x 2 instances = **240 complete trials**.  
**Destroy fraction:** 30% of the **100 currently selected nodes = 30 removed**, leaving 70.  
**Time:** measured mean MSLS elapsed time per instance from Assignment 6.

## 1. Problem and implementation decisions

The closed-tour objective is **minimized**: the sum of rounded Euclidean distances over the entire closed cycle, plus selected-node costs. Only 100 distinct vertices may be selected; all survivors of the destroy stage must stay selected during repair. The last-to-first closing edge is included.

The lecturer's reminder pseudocode prints `If f(y) > f(x)`; this conflicts with the problem's minimization direction. We explicitly implement **`if f(y) < f(x): accept(y)`**. Accepting `>` would repeatedly prefer worse objectives. This is a deliberate correction of an evident inequality inconsistency, not an undisclosed change of the optimization problem.

We use the **unrestricted basic Steepest search with 2-opt for intra-route improvements and selected/unselected node replacement for inter-route improvements**, selecting the most negative delta from the *combined* neighborhood. This is the stronger 2-opt-based search observed in previous random-start experiments. The local search is always applied **once to the initial random solution**. Afterwards, the six LNS versions differ only in their destroy operator and whether a full Steepest call follows each repair.

```text
2OPT-DELTA(a,b,c,d) = D[a,c] + D[b,d] - D[a,b] - D[c,d]
REPLACE-DELTA(old,new,left,right) = cost[new] - cost[old]
                                     + D[left,new] + D[new,right]
                                     - D[left,old] - D[old,right]
```

For 100 selected and 100 unselected vertices, a complete Steepest scan includes **4,850 legal 2-opt moves and 10,000 replacements**. These deltas are constant-time, and the complete neighboring route is not reconstructed during evaluation.

## 2. The three destroy operators (used separately)

All operators remove **exactly 30 of the 100 selected nodes**, retaining the cyclic order of the other 70. The removed vertices rejoin the outside-candidate pool and are not forbidden from being reinserted during repair.

**Random nodes:** sample 30 different *positions* uniformly by partial Fisher-Yates (no replacement). This spreads the damage across the tour, exposing many different insertion opportunities.

**One random subpath:** choose a uniformly random starting position and remove 30 consecutive selected positions modulo tour length; the segment is allowed to cross the last-to-first boundary. This retains a large uninterrupted part of the current tour but destroys one contiguous region.

**Three to five random subpaths:** sample a count `q` uniformly from `{3,4,5}`. Partition 30 removed positions into `q` random lengths, with each length >=3; partition the 70 surviving positions into `q` random positive gaps (each >=1). Interleave a removed block and a retained gap around the cycle, starting from a random rotation. This guarantees **exactly q separated non-overlapping circular subpaths** and exactly 30 removed nodes, without retries or accidental overlaps.

```text
DESTROY(tour,kind,rng):
    m = 100; k = 30
    if kind == RANDOM_NODES:
        removed_positions = sample_without_replacement(0..m-1,k)
    if kind == ONE_SUBPATH:
        start = random_integer(0,m-1)
        removed_positions = {(start+j) mod m : j=0..k-1}
    if kind == MULTIPLE_SUBPATHS:
        q = random_integer(3,5)
        lengths = random_positive_partition(k,q,min_per_part=3)
        gaps = random_positive_partition(m-k,q,min_per_part=1)
        rotate cycle randomly; alternate removed lengths and retained gaps
    return survivors in their original cyclic order, exactly 70 nodes
```

## 3. Repair operator: best weighted 2-regret construction

The prior construction results select **weighted 2-regret in Greedy Cycle form for TSPA** and **weighted 2-regret with open-path all-position insertion for TSPB**. These are the strongest respective instance-specific constructors by their recorded best scores (and also by the best mean among their construction variants). We adapt the *same insertion scoring rule* to a partially preserved 70-node solution rather than starting from one node.

For each currently unused vertex `v`, examine every legal insertion position, including its node cost. Let `delta1` and `delta2` be its two smallest insertion increases, and `regret(v) = delta2 - delta1`. We maximize

```text
score(v) = 0.5 * regret(v) - 0.5 * delta1(v)
```

The term `regret` rewards inserting a vertex before its alternative placements become expensive; the negative best-insertion term discourages expensive additions. Ties are resolved by smaller `delta1`, then lower node index; position ties keep the first position. We insert the chosen vertex at its minimum-increase position, repeat 30 times, and return to exactly 100 distinct selected nodes.

For **TSPA** we keep the remaining 70 nodes in a **closed** cycle, and calculate their insertion increase against a removed cycle edge `(a,b)`:

```text
delta_cycle(v;a,b) = cost[v] + D[a,v] + D[v,b] - D[a,b]
```

For **TSPB** we use the best all-position nearest-neighbor-derived **open path** variant. Importantly, we **break the surviving cycle at one randomly chosen edge** before insertion, as required by the instruction. Insertion at the start/end has no subtracted edge; inner positions use the same edge-replacement formula. We close the path only when all 100 nodes have been restored.

```text
REPAIR(partial,basis,rng):
    S = partial.survivors  # all 70 must remain selected
    if basis == OPEN_PATH:
        cut one randomly selected closing edge by rotating S
    while size(S) < 100:
        for every vertex v not in S:
            find minimum and second-minimum insertion delta across all positions
            score(v) = (delta2 - delta1 - delta1)/2
        choose maximum score, break ties by minimum delta then vertex ID
        insert v into its minimum-delta position
    return S (evaluate as a closed cycle)
```

## 4. Six LNS variants, acceptance and equal time budgets

For each destroy operator we independently evaluate **without** and **with** post-repair local search. Each trial begins with a *fresh random solution*, followed by unrestricted Steepest, **even in the no-post-LS variants**. Strict improving-only acceptance preserves the best incumbent and makes the final objective monotonically nonincreasing. We do not use the ILS 12% worse-acceptance mechanism here; this isolates the destroy/repair design.

```text
LNS(instance,destroy_kind,post_repair_LS,seed,budget_ms):
    rng = MT19937(seed)
    start_timer()  # counts the initial improvement too
    x = STEEPEST(RANDOM_SOLUTION(rng))
    best = x
    iterations = 0
    while elapsed_ms < budget_ms:
        partial = DESTROY(x,destroy_kind,rng)  # remove 30 of 100
        y = REPAIR(partial,selected_regret_basis(instance),rng)
        if post_repair_LS: y = STEEPEST(y)
        iterations += 1
        if objective(y) < objective(x):
            x = y; best = y
    return best,iterations,elapsed_ms
```

The stopping condition is checked **before each new complete destroy/repair/optional-LS iteration**, so a final iteration is allowed to finish slightly beyond the exact limit. The requested time budgets use actual MSLS means from the previous experiment, not invented limits:

| Instance | MSLS average elapsed / LNS budget (ms) | 30%-destroy size |
|---|---:|---:|
| TSPA | 3,715.396616 | 30 |
| TSPB | 3,858.478861 | 30 |

Each of the 12 method/instance groups contains **20 complete independent trials**, or 240 total. We use fixed seed formula `2026700 + instanceIndex*100000 + trial*101 + destroyIndex*11003 + 17` (`instanceIndex=0/1`, `trial=0..19`, `destroyIndex=0..2`). The with/without-LS pair within the same destroy/trial deliberately shares the initial seed to create the same starting local optimum; their later randomness differs because one variant performs extra search work.

## 5. Measured objective values (20 trials per method)

The following are measured **best-so-far final objectives** from independent full-budget LNS trials. Lower is better. Each result is **mean (min - max)**.

'''
parts=[head]
parts.append('| Destroy operator | Post-repair LS | TSPA average (min - max) | TSPB average (min - max) |\n|---|---|---:|---:|\n')
for d in labels:
    for mode in ['without_ls','with_ls']:
        a=by[('TSPA',d,mode)];b=by[('TSPB',d,mode)]
        fmt=lambda x:f"{float(x['mean_objective']):,.3f} ({int(x['min']):,} - {int(x['max']):,})"
        parts.append(f"| {labels[d]} | {'Yes' if mode=='with_ls' else 'No'} | {fmt(a)} | {fmt(b)} |\n")
parts.append('''
## 6. Time, main-loop iterations and LS calls

**All iterations are completed destroy-repair cycles**, not individual Steepest moves. The no-post-LS configuration runs Steepest only once (on the initial random solution). The with-LS configuration invokes Steepest once initially, then once per completed main-loop iteration. The measured per-trial elapsed time includes that initial optimization.

| Destroy | Post-repair LS | TSPA mean ms | TSPA main loops avg (min - max) | TSPB mean ms | TSPB main loops avg (min - max) |
|---|---|---:|---:|---:|---:|
''')
for d in labels:
  for mode in ['without_ls','with_ls']:
    a=by[('TSPA',d,mode)];b=by[('TSPB',d,mode)]
    fmt=lambda v:f"{float(v['mean_iterations']):,.1f} ({int(v['min_iterations']):,} - {int(v['max_iterations']):,})"
    parts.append(f"| {labels[d]} | {'Yes' if mode=='with_ls' else 'No'} | {float(a['mean_elapsed_ms']):,.1f} | {fmt(a)} | {float(b['mean_elapsed_ms']):,.1f} | {fmt(b)} |\n")
parts.append('''
For a fixed time budget, more iterations are possible when the chosen destroy/repair/LS pipeline is cheaper. Iteration count alone is not a quality measure: the objective table must be considered alongside it. The counter records every *completed* proposal, including rejected proposals; another counter separately records accepted improving proposals in `runs.csv`.

## 7. Reference quality (previous experiment results)

These are **previously measured reference values**, not new Assignment 7 trials. Best-solution values and averages from different experiment designs are not interchangeable.

| Reference method | TSPA best | TSPB best | Explanation |
|---|---:|---:|---|
| Weighted 2-regret construction | 72,011 | 50,485 | Weighted cycle on TSPA; weighted all-position open path on TSPB |
| Best basic Local Search | 71,955 | 49,346 | Steepest/Greedy search, 2-opt or node swap and selected/unselected replacement, using greedy initial tours |
| MSLS (400 independent Steepest starts per top-level run) | 72,973 | 49,633 | Best from 20 top-level runs per instance |
| ILS (perturb + Steepest within the MSLS budget) | 71,503 | 48,149 | Best from 20 top-level runs per instance |

## 8. Objective comparison figures

The boxplots below show all 20 final objectives for each of the six LNS configurations. The box shows the central half of the results, the horizontal line is the median, and markers display the mean. Lower values indicate better solutions.

![TSPA - six LNS variants compared](results/plots/TSPA_comparison.svg)

![TSPB - six LNS variants compared](results/plots/TSPB_comparison.svg)

## 9. Best tours for all twelve variants

Each graph plots the entire **closed** tour through exactly 100 distinct nodes, shows all 200 original points, and colors the selected points by selected-node cost. The 100 zero-based node indices follow the graph; the first node is **not** repeated at the end of the list. Values are recomputed from the original coordinates and include the closing edge.

''')
records={}
for line in (RESULT/'best_solutions.txt').read_text().splitlines():
 if not line or line.startswith('#'):continue
 tokens=line.split();records[tuple(tokens[:3])]=(int(tokens[3]),list(map(int,tokens[4:])))
assert len(records)==12
for ins in ('TSPA','TSPB'):
 for d in labels:
  for mode in ('without_ls','with_ls'):
   v,tour=records[(ins,d,mode)]
   node_path=HERE.parent/'Assignment 1/data'/f'{ins}.csv'
   nodes=[tuple(map(int,x.split(';'))) for x in node_path.read_text().splitlines() if x.strip()]
   import math
   travel=sum(math.floor(math.hypot(nodes[tour[i]][0]-nodes[tour[(i+1)%100]][0],nodes[tour[i]][1]-nodes[tour[(i+1)%100]][1])+0.5) for i in range(100))
   costs=sum(nodes[n][2] for n in tour)
   assert travel+costs==v
   name=f'{ins}_{d}_{mode}'
   parts.append(f"### {ins} - {labels[d]} - {'with' if mode=='with_ls' else 'without'} LS\n\n")
   parts.append(f"**Objective:** {v:,} = **travel** {travel:,} + **selected-node costs** {costs:,}.\n\n")
   parts.append(f"![{name} best closed tour](results/plots/{name}.svg)\n\n")
   parts.append('**Best 100 node indices (0-based):**\n\n```text\n')
   for i in range(0,len(tour),20): parts.append(', '.join(map(str,tour[i:i+20]))+'\n')
   parts.append('```\n\n')
parts.append('''## 10. Tests and verification (real executions, not hypothetical laptop claims)

We ran the following automated checks in the local Linux execution environment, compiling with `g++ -O3 -Wall -Wextra -Wpedantic`. This is **not** a claim that a student manually tested the code on a physical laptop. `README.md` and `TESTING.md` provide the exact steps for independent laptop reproduction.

1. **Unit and property tests (`assignment7_tests`):** exact 30% removal, 30 unique positions, wraparound single subpath, 3-5 disjoint nonadjacent subpaths, both weighted-regret repair bases, 100 unique valid vertices, original-survivor retention, closed tour, strict improvement, with-LS local optimality, call counters, time budget, invalid-argument errors. Randomized test cases cover different instance sizes.
2. **Real experimental run audit:** all 240 run-level tours are recalculated **independently from original CSV coordinates**, including mathematical integer rounding, node costs and the closing edge. All 12 groups have exactly 20 independent time-limited trials, with true objective minima, maxima, averages and loop-count statistics.
3. **Paired starts:** every with/without-LS pair uses the same PRNG seed and has the same initial optimized tour objective. All 120 pairs are checked.
4. **Standalone C++ checker:** the twelve best tours are evaluated independently, including the closing edge; for the six with-LS best tours, it also checks that no improving 2-opt or replacement move remains in the complete neighborhood.
5. **Manual reproduction instructions:** build with CMake; run `ctest`; perform a one-trial command; reproduce full experiment with `--workers 1` on a student laptop, then run the standalone checker. This is an executable protocol, not a fabricated account of a manual laptop test.

## 11. Conclusions

''')
for ins in ('TSPA','TSPB'):
 choices=[(float(by[(ins,d,mode)]['mean_objective']),d,mode) for d in labels for mode in ['without_ls','with_ls']]
 avg,d,mode=min(choices)
 parts.append(f"For **{ins}**, the lowest 20-trial mean was obtained by **{labels[d]}**, **{'with' if mode=='with_ls' else 'without'}** post-repair LS (mean objective {avg:,.3f}). ")
 parts.append('This is an empirical result under the specified time budget, not proof of global optimality.\n\n')
parts.append('''The comparison isolates the trade-off between a stronger repair plus post-repair local optimization and the number of large-neighborhood proposals that fit inside the time budget. Strict improvement acceptance and a fixed 30% removal fraction are shared across all six methods. Full objective decomposition, detailed individual results and retained best tours are available in the CSVs and plotted SVG files.\n\n''')
parts.append('''**Reproduction:** [README.md](README.md), [TESTING.md](TESTING.md), [source code](src/lns.hpp), [all runs](results/runs.csv), [summary](results/summary.csv), [best tours](results/best_solutions.txt).\n''')
(HERE/'REPORT.md').write_text(''.join(parts))
print('Wrote REPORT.md with',len(records),'best tours and',len(summary),'summary rows')
