#!/usr/bin/env python3
"""Run the six LNS methods, 20 times each on two instances, and record honest measurements.

This script executes the actual C++ binary. A single reproducible job order mixes
strategies in a local process pool; --workers 1 is suitable for sequential laptop use.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import os
import platform
import random
import subprocess
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from statistics import mean

HERE = Path(__file__).resolve().parents[1]
REPO = HERE.parent
FIELDS = ["instance", "destroy", "ls_mode", "trial", "seed", "budget_ms", "elapsed_ms",
          "initial_objective", "best_objective", "iterations", "accepted", "rejected",
          "local_search_calls", "delta_evaluations", "repair", "best_tour"]


def verify_row(row):
    row = dict(zip(FIELDS, row))
    for key in ["trial", "seed", "initial_objective", "best_objective", "iterations", "accepted",
                "rejected", "local_search_calls", "delta_evaluations"]:
        row[key] = int(row[key])
    for key in ["budget_ms", "elapsed_ms"]:
        row[key] = float(row[key])
    row["tour"] = [int(x) for x in row["best_tour"].split()]
    assert len(row["tour"]) == 100 and len(set(row["tour"])) == 100
    assert row["best_objective"] <= row["initial_objective"]
    assert row["accepted"] + row["rejected"] == row["iterations"]
    assert row["local_search_calls"] == 1 + (row["iterations"] if row["ls_mode"] == "with_ls" else 0)
    assert row["elapsed_ms"] >= row["budget_ms"] or row["iterations"] == 0
    return row


def run_job(job, exe: Path, data: Path, budgets: dict):
    instance, method, trial = job
    command = [str(exe), str(data), instance, str(method), str(trial), f"{budgets[instance]:.6f}"]
    p = subprocess.run(command, capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(f"Command {command!r} failed: {p.stderr}")
    lines = p.stdout.strip().splitlines()
    if len(lines) != 1:
        raise RuntimeError(f"Expected one CSV result, got {lines}")
    row = next(csv.reader(lines))
    if len(row) != len(FIELDS):
        raise RuntimeError(f"Malformed row length {len(row)}: {lines}")
    obj = verify_row(row)
    obj["method_id"] = method
    return obj


def write_outputs(rows, result_dir: Path, budgets: dict, workers: int):
    rows.sort(key=lambda r: (r["instance"], r["method_id"], r["trial"]))
    result_dir.mkdir(exist_ok=True, parents=True)
    with (result_dir/"runs.csv").open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        w.writeheader()
        for r in rows:
            w.writerow({k: (' '.join(map(str, r['tour'])) if k == 'best_tour' else r[k]) for k in FIELDS})
    groups = defaultdict(list)
    for r in rows:
        groups[(r["instance"], r["destroy"], r["ls_mode"])].append(r)
    with (result_dir/"summary.csv").open("w", newline="") as f:
        columns = ["instance", "destroy", "ls_mode", "trials", "min", "max", "mean_objective",
                   "mean_elapsed_ms", "mean_iterations", "min_iterations", "max_iterations",
                   "mean_accepted", "mean_ls_calls", "best_trial"]
        w = csv.DictWriter(f, fieldnames=columns)
        w.writeheader()
        for key, group in sorted(groups.items()):
            best = min(group, key=lambda row: (row["best_objective"], row["trial"]))
            w.writerow(dict(zip(columns, [*key, len(group), best["best_objective"],
                 max(r["best_objective"] for r in group), f"{mean(r['best_objective'] for r in group):.3f}",
                 f"{mean(r['elapsed_ms'] for r in group):.3f}",
                 f"{mean(r['iterations'] for r in group):.3f}",
                 min(r["iterations"] for r in group),max(r["iterations"] for r in group),
                 f"{mean(r['accepted'] for r in group):.3f}",
                 f"{mean(r['local_search_calls'] for r in group):.3f}",best["trial"]])))
    with (result_dir/"best_solutions.txt").open("w") as f:
        f.write("# instance destroy ls_mode objective followed by exactly 100 zero-based indices\n")
        for key, group in sorted(groups.items()):
            best = min(group, key=lambda row: (row["best_objective"], row["trial"]))
            f.write(" ".join([*key,str(best["best_objective"]), *map(str,best["tour"])])+"\n")
    with (result_dir/"budgets.csv").open("w",newline="") as f:
        w=csv.writer(f);w.writerow(["instance","a6_msls_mean_ms","lns_budget_ms","experiment_count_per_method"])
        for dataset, budget in budgets.items():w.writerow([dataset,f"{budget:.6f}",f"{budget:.6f}",20])
    (result_dir/"run_info.json").write_text(json.dumps({
        "system":platform.system(),"platform":platform.platform(),"python":sys.version.split()[0],
        "compiler":subprocess.run(["g++","--version"],capture_output=True,text=True).stdout.splitlines()[0],
        "cpu_count_reported":os.cpu_count(),"workers":workers,
        "experiment_schedule":"4-worker randomized mixed sequence, seed 2026707; use --workers 1 for sequential reproducibility",
        "machine_claim":"Local Linux execution, not a claim of student-operated physical laptop tests",
        "task_budget_source":"Assignment 6/results/budgets.csv (measured MSLS mean wall-clock runtime)",
        "budgets_ms":budgets,
        "number_trials":len(rows),
        "note":"Elapsed times and iteration counts depend on CPU load. Results are real C++ executions."
    },indent=2)+"\n")
    print("Saved",len(rows),"rows and",len(groups),"summary groups",flush=True)


def main():
    arg = argparse.ArgumentParser()
    arg.add_argument("--workers", type=int, default=4)
    arg.add_argument("--trials", type=int, default=20)
    arg.add_argument("--methods", type=int, nargs="*", default=list(range(6)))
    arg.add_argument("--output", type=Path, default=HERE/"results")
    arg.add_argument("--exe",type=Path,default=HERE/"build/assignment7")
    args=arg.parse_args()
    if not 1<=args.trials<=20:raise ValueError("Trials must be 1..20")
    with (REPO/"Assignment 6/results/budgets.csv").open() as f:
        rows=list(csv.DictReader(f))
    budgets={r["instance"]:float(r["msls_mean_ms"]) for r in rows}
    jobs=[(d,m,t) for d in ["TSPA","TSPB"] for m in args.methods for t in range(args.trials)]
    random.Random(2026707).shuffle(jobs)
    result=[]
    print("Starting",len(jobs),"real C++ experiments, workers",args.workers,"budget",budgets,flush=True)
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        tasks={pool.submit(run_job,j,args.exe,REPO/"Assignment 1/data",budgets):j for j in jobs}
        for completed in as_completed(tasks):
            r=completed.result()
            result.append(r)
            if len(result)%10==0 or len(result)==len(jobs):
                print("COMPLETED",len(result),"/",len(jobs),"latest",r["instance"],r["destroy"],
                      r["ls_mode"],"best",r["best_objective"],"iterations",r["iterations"],flush=True)
    if args.trials==20 and args.methods==list(range(6)):
        write_outputs(result,args.output,budgets,args.workers)
    else:
        # Do not publish smoke results in the full experiment paths.
        (args.output/"smoke_results.json").write_text(json.dumps([{k:v for k,v in x.items() if k!='tour'} for x in result],indent=2))

if __name__=="__main__": main()
