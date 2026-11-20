#!/usr/bin/env python3
"""Vector figures for all twelve best tours and two 20-trial comparisons."""
from pathlib import Path
import csv
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parent
OUTPUT=HERE/'results/plots'
OUTPUT.mkdir(parents=True,exist_ok=True)

def read_best():
    records=[]
    for row in (HERE/'results/best_solutions.txt').read_text().splitlines():
        if not row or row.startswith('#'):continue
        parts=row.split()
        records.append((parts[0],parts[1],parts[2],int(parts[3]),[int(v) for v in parts[4:]]))
    return records

def plot_tours():
    for instance,destroy,mode,objective,tour in read_best():
        with (ROOT/'Assignment 1/data'/f'{instance}.csv').open() as f:
            data=np.array([[int(x) for x in line.strip().split(';')] for line in f if line.strip()])
        xy=data[:,:2];cost=data[:,2]
        selected=np.zeros(len(xy),dtype=bool);selected[tour]=True
        travel=sum(int(np.floor(np.hypot(*(xy[tour[i]]-xy[tour[(i+1)%len(tour)]]))+0.5)) for i in range(len(tour)))
        node_cost=sum(int(cost[x]) for x in tour)
        assert travel+node_cost==objective
        fig,ax=plt.subplots(figsize=(7.8,5.25))
        ax.scatter(xy[~selected,0],xy[~selected,1],s=11,color='#D1D5DB',alpha=.45,zorder=1)
        cyc=np.r_[tour,tour[0]]
        ax.plot(xy[cyc,0],xy[cyc,1],lw=.9,color='#344054',alpha=.82,zorder=2)
        points=ax.scatter(xy[tour,0],xy[tour,1],s=21,c=cost[tour],cmap='coolwarm',
                          vmin=0,vmax=max(cost),edgecolors='#344054',linewidths=.35,zorder=3)
        cbar=fig.colorbar(points,ax=ax,shrink=.80,pad=.025);cbar.set_label('Selected-node cost',fontsize=9)
        ax.set_title(f'{instance} | {destroy.replace("_"," ").title()} | {mode.replace("_"," ").title()}\n'
                     f'Objective {objective:,}  =  travel {travel:,} + node costs {node_cost:,}',fontsize=11)
        ax.set_xlabel('X');ax.set_ylabel('Y');ax.set_aspect('equal',adjustable='box')
        ax.grid(alpha=.14,linewidth=.6)
        fig.tight_layout()
        name=f'{instance}_{destroy}_{mode}'
        fig.savefig(OUTPUT/f'{name}.svg',format='svg',bbox_inches='tight')
        fig.savefig(OUTPUT/f'{name}.png',dpi=135,bbox_inches='tight')
        plt.close(fig)
    print('Generated 12 best-tour SVG + PNG plots')

def plot_distributions():
    with (HERE/'results/runs.csv').open() as f: rows=list(csv.DictReader(f))
    for dataset in ['TSPA','TSPB']:
        names=[];values=[]
        for destroy in ['random_nodes','one_subpath','multiple_subpaths']:
            for mode in ['without_ls','with_ls']:
                selection=[int(r['best_objective']) for r in rows if r['instance']==dataset and r['destroy']==destroy and r['ls_mode']==mode]
                assert len(selection)==20
                names.append(f'{destroy.replace("_"," ").title()}\n'+('With LS' if mode=='with_ls' else 'No LS'))
                values.append(selection)
        fig,ax=plt.subplots(figsize=(9,4.2))
        bp=ax.boxplot(values,patch_artist=True,showmeans=True,meanprops={'marker':'o','markersize':4})
        for i,patch in enumerate(bp['boxes']): patch.set_facecolor(plt.get_cmap('tab10')(i)) ; patch.set_alpha(.35)
        ax.set_xticks(range(1,len(names)+1),labels=names,fontsize=8)
        ax.set_ylabel('Final objective (lower is better)')
        ax.set_title(f'{dataset} | 20 independent trials per LNS method')
        ax.grid(axis='y',alpha=.25)
        fig.tight_layout()
        fig.savefig(OUTPUT/f'{dataset}_comparison.svg',bbox_inches='tight')
        fig.savefig(OUTPUT/f'{dataset}_comparison.png',bbox_inches='tight',dpi=160)
        plt.close(fig)
    print('Generated 2 experiment-comparison SVG + PNG plots')

if __name__=='__main__':plot_tours();plot_distributions()
