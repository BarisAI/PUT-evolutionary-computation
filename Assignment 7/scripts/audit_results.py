#!/usr/bin/env python3
"""Independently audit all 240 submitted C++ LNS runs and the aggregate tables."""
from __future__ import annotations
import csv
import math
from collections import defaultdict
from pathlib import Path
from statistics import mean

HERE=Path(__file__).resolve().parents[1]
REPO=HERE.parent

def main():
    instances={}
    for d in ('TSPA','TSPB'):
        with (REPO/'Assignment 1/data'/f'{d}.csv').open() as f:
            instances[d]=[tuple(map(int,r.strip().split(';'))) for r in f if r.strip()]
        assert len(instances[d])==200
    with (HERE/'results/runs.csv').open() as f:records=list(csv.DictReader(f))
    assert len(records)==240,len(records)
    groups=defaultdict(list)
    starts=defaultdict(dict)
    for row in records:
        d,destroy,mode=row['instance'],row['destroy'],row['ls_mode']
        trial=int(row['trial']);tour=[int(n) for n in row['best_tour'].split()]
        assert d in ('TSPA','TSPB') and destroy in ('random_nodes','one_subpath','multiple_subpaths')
        assert mode in ('with_ls','without_ls') and 0<=trial<20
        assert len(tour)==100 and len(set(tour))==100 and all(0<=n<200 for n in tour)
        nodes=instances[d]
        travel=sum(math.floor(math.hypot(nodes[tour[i]][0]-nodes[tour[(i+1)%100]][0],nodes[tour[i]][1]-nodes[tour[(i+1)%100]][1])+0.5) for i in range(100))
        cost=sum(nodes[n][2] for n in tour)
        assert travel+cost==int(row['best_objective']),row
        assert int(row['best_objective'])<=int(row['initial_objective'])
        assert int(row['accepted'])+int(row['rejected'])==int(row['iterations'])
        assert int(row['local_search_calls'])==1+(int(row['iterations']) if mode=='with_ls' else 0)
        assert float(row['elapsed_ms'])>=float(row['budget_ms'])
        b=3715.396616 if d=='TSPA' else 3858.478861
        assert abs(float(row['budget_ms'])-b)<1e-5
        assert row['repair']==('weighted_cycle' if d=='TSPA' else 'weighted_open_path')
        expected=2026700+(0 if d=='TSPA' else 1)*100000+trial*101+['random_nodes','one_subpath','multiple_subpaths'].index(destroy)*11003+17
        assert int(row['seed'])==expected
        groups[(d,destroy,mode)].append(row)
        starts[(d,destroy,trial)][mode]=int(row['initial_objective'])
    assert len(groups)==12 and all(len(g)==20 for g in groups.values())
    assert len(starts)==120 and all(len(g)==2 and g['with_ls']==g['without_ls'] for g in starts.values())
    with (HERE/'results/summary.csv').open() as f:summary=list(csv.DictReader(f))
    assert len(summary)==12
    for s in summary:
        g=groups[(s['instance'],s['destroy'],s['ls_mode'])]
        vals=[int(r['best_objective']) for r in g]
        counts=[int(r['iterations']) for r in g]
        assert int(s['trials'])==20
        assert int(s['min'])==min(vals) and int(s['max'])==max(vals)
        assert abs(float(s['mean_objective'])-mean(vals))<0.0006
        assert abs(float(s['mean_iterations'])-mean(counts))<0.0006
        assert int(s['min_iterations'])==min(counts) and int(s['max_iterations'])==max(counts)
        assert abs(float(s['mean_elapsed_ms'])-mean(float(r['elapsed_ms']) for r in g))<.0006
    best=[ln for ln in (HERE/'results/best_solutions.txt').read_text().splitlines() if ln and ln[0]!='#']
    assert len(best)==12
    for line in best:
        tok=line.split();g=groups[tuple(tok[:3])];minval=min(int(r['best_objective']) for r in g)
        assert int(tok[3])==minval
        assert len(tok[4:])==100
        assert any(int(r['best_objective'])==minval and r['best_tour']==" ".join(tok[4:]) for r in g)
    print('PASS: 240/240 independently re-evaluated with rounded closed edges and selected node costs.')
    print('PASS: 12/12 groups have 20 real trials, summary statistics and loop counts verified.')
    print('PASS: 120/120 paired trials have the same initial solution objective and seed.')
    print('PASS: 12/12 saved best tours match run-level winners and all measured time budgets.')

if __name__=='__main__':main()
