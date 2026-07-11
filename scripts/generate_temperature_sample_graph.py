import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({
    "font.size": 11,
    "axes.edgecolor": "#333333",
    "axes.labelcolor": "#222222",
})

t = np.linspace(0, 60, 200)  # minutes

# Without BTMS: temperature keeps climbing under continuous discharge load
temp_no_btms = 28 + 0.75*t + 0.004*t**2
temp_no_btms = np.clip(temp_no_btms, None, 78)

# With BTMS: fan cooling kicks in at 45C and cycles, holding pack near 40-46C
temp_with_btms = 28 + 22*(1 - np.exp(-t/10))
ripple = 2.2*np.sin(t/2.1) * (t > 15)
temp_with_btms = temp_with_btms + ripple
temp_with_btms = np.clip(temp_with_btms, None, 47)

fig, ax = plt.subplots(figsize=(9, 5.2), dpi=150)

ax.plot(t, temp_no_btms, color="#d1495b", linewidth=2.4, label="Without BTMS (fan off)")
ax.plot(t, temp_with_btms, color="#2a9d8f", linewidth=2.4, label="With BTMS active (fan + cutoff control)")

ax.axhline(45, color="#e9a941", linestyle="--", linewidth=1.4, label="Warning threshold (45°C)")
ax.axhline(60, color="#c1121f", linestyle="--", linewidth=1.4, label="Critical threshold (60°C) — auto cutoff")

ax.set_xlabel("Time (minutes) — continuous discharge test")
ax.set_ylabel("Pack Temperature (°C)")
ax.set_title("Sample Result: Battery Pack Temperature vs Time\n(With vs Without BTMS)", fontsize=13, fontweight="bold")
ax.set_xlim(0, 60)
ax.set_ylim(20, 85)
ax.grid(True, alpha=0.3)
ax.legend(loc="upper left", frameon=True, fontsize=9.5)

fig.text(0.5, -0.02, "Illustrative sample data — replace with your own logged test results (see /data/test_logs.csv)",
          ha="center", fontsize=8.5, style="italic", color="#555555")

plt.tight_layout()
plt.savefig("../assets/temperature_comparison_graph.png", bbox_inches="tight", facecolor="white")
print("saved")
