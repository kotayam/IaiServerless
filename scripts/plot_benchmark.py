#!/usr/bin/env python3
"""Generate benchmark graphs from benchmark_result.csv."""
import csv
import sys
import os
import matplotlib.pyplot as plt
import numpy as np
from collections import OrderedDict

COLORS = {
    "process": "#4CAF50", "python": "#FF9800", "kvm": "#2196F3",
    "docker": "#F44336", "junction": "#9C27B0",
}

def load_csv(path):
    """Load CSV into {function: {runtime: {metric: value}}}."""
    data = OrderedDict()
    with open(path) as f:
        for row in csv.DictReader(f):
            fn = row["function"]
            rt = row["runtime"]
            data.setdefault(fn, OrderedDict())[rt] = {
                "cold_start_ms": float(row["cold_start_ms"]) if row["cold_start_ms"] else None,
                "exec_time_ms": float(row["exec_time_ms"]) if row["exec_time_ms"] else None,
                "e2e_ms": float(row["e2e_ms"]) if row["e2e_ms"] else None,
                "binary_size_bytes": int(row["binary_size_bytes"]) if row["binary_size_bytes"] else None,
            }
    return data

def split_by_lang(data):
    c = OrderedDict((k, v) for k, v in data.items() if k.startswith("c/"))
    py = OrderedDict((k, v) for k, v in data.items() if k.startswith("python/"))
    return c, py

def get_runtimes(group):
    rts = []
    for runtimes in group.values():
        for rt in runtimes:
            if rt not in rts:
                rts.append(rt)
    return rts

def plot_metric(ax, group, metric, title, ylabel, log_scale=False):
    functions = list(group.keys())
    runtimes = get_runtimes(group)
    x = np.arange(len(functions))
    n = len(runtimes)
    width = 0.8 / n

    for i, rt in enumerate(runtimes):
        vals = []
        mask = []
        for fn in functions:
            v = group[fn].get(rt, {}).get(metric)
            vals.append(v if v is not None else 0)
            mask.append(v is not None)
        bars = ax.bar(x + i * width - 0.4 + width / 2, vals, width,
                      label=rt, color=COLORS.get(rt, "#999"), zorder=3)
        for j, b in enumerate(bars):
            if not mask[j]:
                b.set_alpha(0.15)

    ax.set_title(title, fontsize=11, fontweight="bold")
    ax.set_ylabel(ylabel)
    ax.set_xticks(x)
    ax.set_xticklabels(functions, rotation=30, ha="right", fontsize=8)
    ax.legend(fontsize=7)
    ax.grid(axis="y", alpha=0.3, zorder=0)
    if log_scale:
        ax.set_yscale("log")

def plot_group(group, lang_label, out_path):
    # Convert binary_size_bytes to KB for display
    group_kb = OrderedDict()
    for fn, runtimes in group.items():
        group_kb[fn] = OrderedDict()
        for rt, metrics in runtimes.items():
            group_kb[fn][rt] = dict(metrics)
            bs = metrics.get("binary_size_bytes")
            group_kb[fn][rt]["size_kb"] = bs / 1024 if bs else None

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f"{lang_label} — Benchmark Comparison", fontsize=14, fontweight="bold")

    plot_metric(axes[0, 0], group, "cold_start_ms", "Cold Start", "ms", log_scale=True)
    plot_metric(axes[0, 1], group, "exec_time_ms", "Execution Time", "ms", log_scale=True)
    plot_metric(axes[1, 0], group, "e2e_ms", "E2E Latency", "ms", log_scale=True)
    plot_metric(axes[1, 1], group_kb, "size_kb", "Binary / Image Size", "KB", log_scale=True)

    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    print(f"Saved {out_path}")

def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "scripts/benchmark_result.csv"
    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found. Run benchmark.py first.")
        sys.exit(1)

    data = load_csv(csv_path)
    c_group, py_group = split_by_lang(data)

    out_dir = os.path.dirname(csv_path) or "."
    if c_group:
        plot_group(c_group, "C Functions", os.path.join(out_dir, "bench_c_functions.png"))
    if py_group:
        plot_group(py_group, "Python Functions", os.path.join(out_dir, "bench_py_functions.png"))

if __name__ == "__main__":
    main()
