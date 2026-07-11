"""
Quick helper to plot your own logged BTMS test data.

Expected CSV format (data/test_logs.csv), one row per reading:
    timestamp_min,condition,temperature_c
    0,without_btms,28.0
    1,without_btms,29.1
    ...
    0,with_btms,28.0
    1,with_btms,28.6
    ...

Usage:
    python3 plot_results.py
"""
import csv
import matplotlib.pyplot as plt
from collections import defaultdict

DATA_FILE = "../data/test_logs.csv"

def load_data(path):
    series = defaultdict(list)
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            t = float(row["timestamp_min"])
            temp = float(row["temperature_c"])
            series[row["condition"]].append((t, temp))
    return series

def main():
    series = load_data(DATA_FILE)
    fig, ax = plt.subplots(figsize=(9, 5))
    for condition, points in series.items():
        points.sort()
        xs = [p[0] for p in points]
        ys = [p[1] for p in points]
        ax.plot(xs, ys, marker="o", markersize=3, label=condition)

    ax.axhline(45, linestyle="--", color="orange", label="Warning (45C)")
    ax.axhline(60, linestyle="--", color="red", label="Critical (60C)")
    ax.set_xlabel("Time (minutes)")
    ax.set_ylabel("Temperature (C)")
    ax.set_title("BTMS Test Results")
    ax.legend()
    ax.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig("../assets/my_test_results.png", dpi=150)
    print("Saved to ../assets/my_test_results.png")

if __name__ == "__main__":
    main()
