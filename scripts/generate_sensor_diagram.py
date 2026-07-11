import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.lines import Line2D

fig, ax = plt.subplots(figsize=(10, 6.5), dpi=150)
ax.set_xlim(0, 10)
ax.set_ylim(0, 6.5)
ax.axis("off")

# Battery pack outer box
pack = patches.FancyBboxPatch((0.6, 1.4), 5.4, 3.4, boxstyle="round,pad=0.02,rounding_size=0.08",
                               linewidth=2, edgecolor="#1d3557", facecolor="#e8f0fe")
ax.add_patch(pack)
ax.text(3.3, 5.1, "48V / 50Ah Li-ion Battery Pack (~2.4 kWh)", ha="center", fontsize=11.5, fontweight="bold", color="#1d3557")
ax.text(3.3, 4.75, "4 Modules in series (M1-M4)", ha="center", fontsize=9.5, color="#333333")

module_labels = ["M1", "M2", "M3", "M4"]
sensor_labels = ["T1", "T2", "T3", "T4"]
mod_x = [0.95, 2.25, 3.55, 4.85]
for i in range(4):
    mod = patches.FancyBboxPatch((mod_x[i], 1.7), 1.1, 2.7, boxstyle="round,pad=0.02,rounding_size=0.05",
                                  linewidth=1.3, edgecolor="#457b9d", facecolor="#ffffff")
    ax.add_patch(mod)
    ax.text(mod_x[i]+0.55, 3.9, module_labels[i], ha="center", fontsize=10, fontweight="bold", color="#457b9d")
    # temp sensor dot
    circ = patches.Circle((mod_x[i]+0.55, 2.15), 0.14, facecolor="#e63946", edgecolor="black", linewidth=0.8, zorder=5)
    ax.add_patch(circ)
    ax.text(mod_x[i]+0.55, 1.85, sensor_labels[i], ha="center", fontsize=8.5, fontweight="bold")

ax.text(3.3, 1.55, "DS18B20 waterproof temperature sensors (one per module, taped to cell surface)",
        ha="center", fontsize=8, style="italic", color="#444444")

# Current sensor near output terminal
cur_box = patches.FancyBboxPatch((6.4, 3.6), 2.1, 0.85, boxstyle="round,pad=0.02,rounding_size=0.06",
                                  linewidth=1.3, edgecolor="#e76f51", facecolor="#fff1ea")
ax.add_patch(cur_box)
ax.text(7.45, 4.02, "ACS712 Current Sensor\n(in series with +ve terminal)", ha="center", fontsize=8.5)

# Voltage sensor across pack
volt_box = patches.FancyBboxPatch((6.4, 2.5), 2.1, 0.85, boxstyle="round,pad=0.02,rounding_size=0.06",
                                   linewidth=1.3, edgecolor="#e76f51", facecolor="#fff1ea")
ax.add_patch(volt_box)
ax.text(7.45, 2.92, "Voltage Sensor Module\n(across pack terminals)", ha="center", fontsize=8.5)

# Gas sensor at vent
gas_box = patches.FancyBboxPatch((6.4, 1.4), 2.1, 0.85, boxstyle="round,pad=0.02,rounding_size=0.06",
                                  linewidth=1.3, edgecolor="#e76f51", facecolor="#fff1ea")
ax.add_patch(gas_box)
ax.text(7.45, 1.82, "MQ-2 Gas/Smoke Sensor\n(at pack vent, thermal-runaway cue)", ha="center", fontsize=8.5)

# ESP32 controller box
ctrl = patches.FancyBboxPatch((3.2, 5.55), 3.0, 0.7, boxstyle="round,pad=0.02,rounding_size=0.06",
                               linewidth=1.6, edgecolor="#2a9d8f", facecolor="#e6f6f4")
ax.add_patch(ctrl)
ax.text(4.7, 5.9, "ESP32 Controller (BTMS Logic + WiFi Logging)", ha="center", fontsize=9, fontweight="bold", color="#2a9d8f")

# connecting lines from sensors to controller
def connector(x1, y1, x2, y2):
    ax.add_line(Line2D([x1, x2], [y1, y2], color="#888888", linewidth=1, linestyle=(0, (3, 2))))

for i in range(4):
    connector(mod_x[i]+0.55, 2.29, 4.7, 5.55)
connector(7.45, 4.45, 4.7, 5.55)
connector(7.45, 3.35, 4.7, 5.55)
connector(7.45, 2.25, 4.7, 5.55)

ax.set_title("Sensor Placement Schematic — Battery Pack + Monitoring Layer", fontsize=13, fontweight="bold", pad=20)

plt.tight_layout()
plt.savefig("../assets/sensor_placement_diagram.png", bbox_inches="tight", facecolor="white")
print("saved")
