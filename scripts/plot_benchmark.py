#!/usr/bin/env python3
"""Generate benchmark graphs from benchmark_result.csv."""
import csv
import sys
import os
import matplotlib.pyplot as plt
import numpy as np
from collections import OrderedDict

COLORS = {
    "process": "#4CAF50", "python": "#FF9800", "kvm": "#F44336",
    "docker": "#2196F3", "junction": "#9C27B0",
}

def load_csv(path):
    data = OrderedDict()
    with open(path) as f:
        for row in csv.DictReader(f):
            fn = row["function"]
            rt = row["runtime"]
            data.setdefault(fn, OrderedDict())[rt] = {
                "cold_start_ms": float(row["cold_start_ms"]) if row["cold_start_ms"] else None,
                "exec_time_ms": float(row["exec_time_ms"]) if row["exec_time_ms"] else None,
                "e2e_ms": float(row["e2e_ms"]) if row["e2e_ms"] else None,
                "size_kb": int(row["binary_size_bytes"]) / 1024 if row["binary_size_bytes"] else None,
            }
    return data

def get_runtimes(data):
    rts = []
    for runtimes in data.values():
        for rt in runtimes:
            if rt not in rts:
                rts.append(rt)
    return rts

def plot_metric(data, metric, title, ylabel, out_path):
    functions = list(data.keys())
    runtimes = get_runtimes(data)
    x = np.arange(len(functions))
    n = len(runtimes)
    width = 0.8 / n

    fig, ax = plt.subplots(figsize=(max(14, len(functions) * 1.2), 6))

    for i, rt in enumerate(runtimes):
        vals = []
        mask = []
        for fn in functions:
            v = data[fn].get(rt, {}).get(metric)
            vals.append(v if v is not None else 0)
            mask.append(v is not None)
        color = COLORS.get(rt, "#999")
        if rt == "kvm":
            bars = ax.bar(x + i * width - 0.4 + width / 2, vals, width,
                          label=rt, color=color, edgecolor=color, linewidth=1.2, zorder=3)
        else:
            plt.rcParams['hatch.color'] = color
            bars = ax.bar(x + i * width - 0.4 + width / 2, vals, width,
                          label=rt, facecolor="none", edgecolor=color,
                          hatch="////", linewidth=1.2, zorder=3)
        for j, b in enumerate(bars):
            if not mask[j]:
                b.set_alpha(0.15)

    ax.set_title(title, fontsize=14, fontweight="bold")
    ax.set_ylabel(ylabel)
    ax.set_xticks(x)
    ax.set_xticklabels(functions, rotation=35, ha="right", fontsize=9)
    ax.legend(fontsize=9)
    ax.grid(axis="y", alpha=0.3, zorder=0)

    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    plt.close()
    print(f"Saved {out_path}")

def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "scripts/benchmark_result.csv"
    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found. Run benchmark.py first.")
        sys.exit(1)

    data = load_csv(csv_path)
    out_dir = os.path.dirname(csv_path) or "."

    plot_metric(data, "cold_start_ms", "Cold Start Latency", "ms",
                os.path.join(out_dir, "bench_cold_start.png"))
    plot_metric(data, "exec_time_ms", "Execution Time", "ms",
                os.path.join(out_dir, "bench_exec_time.png"))
    plot_metric(data, "e2e_ms", "End-to-End Latency", "ms",
                os.path.join(out_dir, "bench_e2e.png"))
    plot_metric(data, "size_kb", "Binary / Image Size", "KB",
                os.path.join(out_dir, "bench_binary_size.png"))

if __name__ == "__main__":
    main()
