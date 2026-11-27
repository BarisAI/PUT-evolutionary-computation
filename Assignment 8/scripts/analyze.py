#!/usr/bin/env python3
"""Analyze 1,000 Greedy local optima per instance; create 12 objective/similarity charts."""
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
import math
from pathlib import Path
import statistics

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np


def nodes_of(tour):
    return frozenset(tour)


def edges_of(tour):
    return frozenset(tuple(sorted((tour[i], tour[(i + 1) % len(tour)])))
                     for i in range(len(tour)))


def mean_overlap(sets):
    """The average overlap with all OTHER optima; duplicate solutions remain runs."""
    if len(sets) <= 1:
        raise ValueError('Need at least two optima')
    counts = Counter(feature for solution in sets for feature in solution)
    return [sum(counts[feature] - 1 for feature in solution) / (len(sets) - 1)
            for solution in sets]


def overlaps(sets, ref):
    return [len(a & ref) for a in sets]


def pearson(x, y):
    x, y = np.asarray(x, dtype=float), np.asarray(y, dtype=float)
    if len(x) != len(y) or len(x) < 3:
        raise ValueError('Need paired samples')
    if np.std(x) == 0 or np.std(y) == 0:
        return float('nan')
    return float(np.corrcoef(x, y)[0, 1])


def independently_score(tour, nodes):
    if len(tour) != 100 or len(set(tour)) != 100 or any(v < 0 or v >= len(nodes) for v in tour):
        raise ValueError('Invalid tour')
    return sum(nodes[u][2] + math.floor(math.hypot(nodes[u][0] - nodes[v][0],
                                                 nodes[u][1] - nodes[v][1]) + 0.5)
               for u, v in zip(tour, tour[1:] + tour[:1]))


def load_instance(path):
    return [tuple(map(int, line.strip().split(';'))) for line in path.read_text().splitlines() if line.strip()]


def load_previous_references(path):
    ans = {}
    for line in path.read_text().splitlines():
        if line.startswith('#') or not line.strip():
            continue
        bits = line.split()
        if bits[1] == 'ILS':
            ans[bits[0]] = {'objective': int(bits[2]), 'tour': list(map(int, bits[3:]))}
    return ans


def verify_average(sets, calculated, samples=(0, 1, 5, 37, 201, 585, 999)):
    for i in samples:
        if i >= len(sets):
            continue
        manual = sum(len(sets[i] & sets[j]) for j in range(len(sets)) if i != j) / (len(sets) - 1)
        if not math.isclose(manual, calculated[i], abs_tol=1e-12):
            raise AssertionError(f'Frequency and brute-force overlap disagree at {i}')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--data', type=Path, default=Path('../Assignment 1/data'))
    p.add_argument('--reference', type=Path, default=Path('../Assignment 6/results/best_solutions.txt'))
    p.add_argument('--input', type=Path, default=Path('results/local_optima.csv'))
    p.add_argument('--output', type=Path, default=Path('results'))
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    charts = args.output / 'charts'
    charts.mkdir(parents=True, exist_ok=True)
    reference_tours = load_previous_references(args.reference)
    gathered = defaultdict(list)
    with args.input.open(newline='') as f:
        for row in csv.DictReader(f):
            row['run'] = int(row['run'])
            row['objective'] = int(row['objective'])
            row['tour'] = [int(x) for x in row['tour'].split()]
            gathered[row['instance']].append(row)

    records = []
    metrics = []
    summary = []
    references_lines = ['# instance reference objective source then 100 zero-based node indices']
    for instance in ('TSPA', 'TSPB'):
        entries = gathered[instance]
        if len(entries) != 1000:
            raise AssertionError(f'{instance}: expected 1000, got {len(entries)}')
        assert [x['run'] for x in entries] == list(range(1000))
        node_data = load_instance(args.data / f'{instance}.csv')
        if len(node_data) != 200:
            raise AssertionError('Expected 200 nodes')
        for e in entries:
            objective = independently_score(e['tour'], node_data)
            if objective != e['objective']:
                raise AssertionError(f'{instance} run {e["run"]}: objective mismatch {objective}')
        ref = reference_tours[instance]
        if independently_score(ref['tour'], node_data) != ref['objective']:
            raise AssertionError(f'{instance}: external reference objective mismatch')
        refidx = min(range(len(entries)), key=lambda idx: entries[idx]['objective'])
        good = entries[refidx]
        scores = [e['objective'] for e in entries]
        references_lines.append(f"{instance} best_of_1000 {good['objective']} sample_run={refidx} " + ' '.join(map(str, good['tour'])))
        references_lines.append(f"{instance} best_previous_ILS {ref['objective']} Assignment_6 " + ' '.join(map(str, ref['tour'])))
        summary.append({'instance': instance, 'n': 1000,
                        'distinct_tours': len({tuple(e['tour']) for e in entries}),
                        'distinct_node_sets': len({nodes_of(e['tour']) for e in entries}),
                        'minimum': min(scores), 'mean': statistics.fmean(scores), 'maximum': max(scores),
                        'best_run': refidx, 'previous_ils_objective': ref['objective']})

        for measure, getfeatures in (('edges', edges_of), ('nodes', nodes_of)):
            sets = [getfeatures(e['tour']) for e in entries]
            if any(len(s) != 100 for s in sets):
                raise AssertionError('Expected 100 selected nodes and 100 undirected edges')
            verify_average(sets, mean_overlap(sets))
            comparisons = [('average_to_others', mean_overlap(sets), None),
                           ('best_of_1000', overlaps(sets, sets[refidx]), refidx),
                           ('best_previous_ILS', overlaps(sets, getfeatures(ref['tour'])), None)]
            for reference, similarities, exclude_run in comparisons:
                indices = [i for i in range(len(entries)) if i != exclude_run]
                x = [scores[i] for i in indices]
                y = [similarities[i] for i in indices]
                r = pearson(x, y)
                metrics.append({'instance': instance, 'measure': measure, 'reference': reference,
                                'points': len(indices), 'pearson_r': r,
                                'mean_similarity': statistics.fmean(y),
                                'min_similarity': min(y), 'max_similarity': max(y)})
                for i in indices:
                    records.append({'instance':instance,'measure':measure,'reference':reference,
                                    'run':i,'objective':scores[i], 'similarity':f'{similarities[i]:.6f}'})
                fig, ax = plt.subplots(figsize=(6.6,4.2))
                ax.scatter(x, y, s=12, alpha=.42, linewidths=0)
                ax.set_xlabel('Objective function value (lower is better)')
                yword = 'common undirected edges' if measure == 'edges' else 'common selected nodes'
                ax.set_ylabel(('Average ' if reference == 'average_to_others' else '') + yword)
                ax.set_title(f'{instance} - {measure}: {reference.replace("_", " ")}')
                ax.text(.98,.96, f'Pearson r = {r:+.3f}  |  n = {len(x)}',
                        transform=ax.transAxes, ha='right',va='top',fontsize=9,
                        bbox={'facecolor':'white','alpha':.88,'edgecolor':'none'})
                ax.grid(alpha=.2)
                fig.tight_layout()
                fig.savefig(charts / f'{instance}_{measure}_{reference}.svg', format='svg')
                plt.close(fig)

    with (args.output / 'metrics.csv').open('w', newline='') as f:
        w=csv.DictWriter(f, fieldnames=['instance','measure','reference','points','pearson_r',
                                        'mean_similarity','min_similarity','max_similarity'])
        w.writeheader();w.writerows(metrics)
    with (args.output / 'optima_summary.csv').open('w', newline='') as f:
        w=csv.DictWriter(f, fieldnames=['instance','n','distinct_tours','distinct_node_sets',
                                        'minimum','mean','maximum','best_run','previous_ils_objective'])
        w.writeheader();w.writerows(summary)
    with (args.output / 'points.csv').open('w', newline='') as f:
        w=csv.DictWriter(f, fieldnames=['instance','measure','reference','run','objective','similarity'])
        w.writeheader();w.writerows(records)
    (args.output / 'references.txt').write_text('\n'.join(references_lines)+'\n')
    for x in summary: print('SUMMARY', x)
    for x in metrics: print('CORR', x['instance'], x['measure'], x['reference'],
                            'n=', x['points'], 'r=', f"{x['pearson_r']:+.4f}")
    print('PASS: 2000 tours independently evaluated; 12 scatter plots; self excluded correctly; average overlaps cross-checked')


if __name__ == '__main__':
    main()
