"""
monitor.py - Live plot of photodiode (input) and AOM drive (output) voltages.

Reads the two-column ASCII stream from the Pico's USB serial port and displays
a scrolling matplotlib plot.

Usage:
    python monitor.py              # auto-detect serial port
    python monitor.py COM5         # explicit port on Windows
    python monitor.py /dev/ttyACM0 # explicit port on Linux
"""

import sys
import glob
import time
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

BAUD = 115200
HISTORY = 500  # number of samples shown on screen


def find_serial_port():
    """Return the first Pico-like serial port found, or None."""
    if sys.platform.startswith("win"):
        candidates = [f"COM{i}" for i in range(1, 20)]
    elif sys.platform.startswith("linux"):
        candidates = glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")
    elif sys.platform.startswith("darwin"):
        candidates = glob.glob("/dev/tty.usbmodem*") + glob.glob("/dev/cu.usbmodem*")
    else:
        candidates = []

    for port in candidates:
        try:
            s = serial.Serial(port, BAUD, timeout=0.1)
            s.close()
            return port
        except (serial.SerialException, OSError):
            continue
    return None


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_serial_port()
    if port is None:
        print("ERROR: no serial port found. Pass the port as an argument.")
        sys.exit(1)

    print(f"Opening {port} at {BAUD} baud ...")
    ser = serial.Serial(port, BAUD, timeout=0.05)
    time.sleep(2)  # wait for Pico to finish its startup sleep

    input_v = deque(maxlen=HISTORY)
    output_v = deque(maxlen=HISTORY)
    sample_idx = deque(maxlen=HISTORY)
    idx = 0

    fig, (ax_in, ax_out) = plt.subplots(2, 1, sharex=True, figsize=(10, 6))
    fig.suptitle("Laser Intensity Stabilizer")

    (line_in,) = ax_in.plot([], [], color="#2196F3", linewidth=1)
    ax_in.set_ylabel("Photodiode (V)")
    ax_in.set_ylim(-0.1, 3.4)
    ax_in.axhline(y=float("nan"), color="gray", linestyle="--", linewidth=0.8)

    (line_out,) = ax_out.plot([], [], color="#FF5722", linewidth=1)
    ax_out.set_ylabel("AOM Drive (V)")
    ax_out.set_ylim(-0.1, 3.4)
    ax_out.set_xlabel("Sample")

    setpoint_line = None  # drawn once we read the header

    def update(_frame):
        nonlocal idx, setpoint_line
        while ser.in_waiting:
            raw = ser.readline()
            try:
                line = raw.decode("utf-8", errors="replace").strip()
            except Exception:
                continue
            if not line:
                continue

            if line.startswith("#"):
                # Parse setpoint from header if present
                if "setpoint_V=" in line:
                    sp = float(line.split("setpoint_V=")[1].split()[0])
                    if setpoint_line is None:
                        setpoint_line = ax_in.axhline(
                            y=sp, color="gray", linestyle="--", linewidth=0.8,
                            label=f"setpoint {sp:.2f} V",
                        )
                        ax_in.legend(loc="upper right", fontsize=8)
                print(line)
                continue

            parts = line.split()
            if len(parts) < 2:
                continue
            try:
                iv = float(parts[0])
                ov = float(parts[1])
            except ValueError:
                continue

            input_v.append(iv)
            output_v.append(ov)
            sample_idx.append(idx)
            idx += 1

        if len(sample_idx) > 0:
            line_in.set_data(list(sample_idx), list(input_v))
            line_out.set_data(list(sample_idx), list(output_v))
            ax_in.set_xlim(sample_idx[0], sample_idx[-1] + 1)
            ax_out.set_xlim(sample_idx[0], sample_idx[-1] + 1)

        return line_in, line_out

    ani = animation.FuncAnimation(fig, update, interval=50, blit=False, cache_frame_data=False)
    plt.tight_layout()
    plt.show()

    ser.close()


if __name__ == "__main__":
    main()
