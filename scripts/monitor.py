"""
monitor.py - Live plot of photodiode (input), AOM drive (output), and setpoint.

Reads a three-column ASCII stream from the Pico's USB serial port (optional
two-column for older firmware) and displays a scrolling matplotlib plot.

Usage:
    python monitor.py              # auto-detect serial port
    python monitor.py COM5         # explicit port on Windows
    python monitor.py /dev/ttyACM0 # explicit port on Linux
"""

import math
import sys
import glob
import time
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

BAUD = 115200
HISTORY = 500  # number of samples shown on screen


PICO_VID_PID = ("2E8A",)  # Raspberry Pi VID


def find_serial_port():
    """Return the first Pico-like serial port found, or None.

    Prefers ports whose USB VID matches the Raspberry Pi vendor ID so we
    don't accidentally open an unrelated serial adapter.
    """
    import serial.tools.list_ports

    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if p.vid is not None and f"{p.vid:04X}" in PICO_VID_PID:
            return p.device

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
    setpoint_v = deque(maxlen=HISTORY)
    sample_idx = deque(maxlen=HISTORY)
    idx = 0

    fig, (ax_in, ax_out) = plt.subplots(2, 1, sharex=True, figsize=(10, 6))
    fig.suptitle("Laser Intensity Stabilizer")

    (line_in,) = ax_in.plot([], [], color="#2196F3", linewidth=1, label="photodiode")
    (line_sp,) = ax_in.plot(
        [], [], color="#4CAF50", linewidth=1, linestyle="--", label="setpoint"
    )
    ax_in.set_ylabel("Photodiode / setpoint (V)")
    ax_in.legend(loc="upper right", fontsize=8)

    (line_out,) = ax_out.plot([], [], color="#FF5722", linewidth=1)
    ax_out.set_ylabel("AOM Drive (V)")
    ax_out.set_ylim(0.1, 1.3)
    ax_out.set_xlabel("Sample")

    setpoint_latest = None  # from header or telemetry

    def update(_frame):
        nonlocal idx, setpoint_latest
        while ser.in_waiting:
            raw = ser.readline()
            try:
                line = raw.decode("utf-8", errors="replace").strip()
            except Exception:
                continue
            if not line:
                continue

            if line.startswith("#"):
                if "setpoint_V=" in line:
                    sp = float(line.split("setpoint_V=")[1].split()[0])
                    setpoint_latest = sp
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

            if len(parts) >= 3:
                try:
                    sp = float(parts[2])
                    setpoint_latest = sp
                except ValueError:
                    sp = setpoint_latest
            else:
                sp = setpoint_latest

            if sp is None:
                sp = float("nan")

            input_v.append(iv)
            output_v.append(ov)
            setpoint_v.append(sp)
            sample_idx.append(idx)
            idx += 1

        if len(sample_idx) > 0:
            line_in.set_data(list(sample_idx), list(input_v))
            line_out.set_data(list(sample_idx), list(output_v))
            line_sp.set_data(list(sample_idx), list(setpoint_v))
            ax_in.set_xlim(sample_idx[0], sample_idx[-1] + 1)
            ax_out.set_xlim(sample_idx[0], sample_idx[-1] + 1)

            # Auto-scale photodiode Y to visible signal + setpoint (5% margin)
            ys = [
                y
                for y in list(input_v) + list(setpoint_v)
                if not math.isnan(y)
            ]
            if ys:
                lo, hi = min(ys), max(ys)
                span = hi - lo
                margin = max(0.02, span * 0.05) if span > 0 else 0.05
                ax_in.set_ylim(lo - margin, hi + margin)

        return line_in, line_sp, line_out

    ani = animation.FuncAnimation(fig, update, interval=50, blit=False, cache_frame_data=False)
    plt.tight_layout()
    plt.show()

    ser.close()


if __name__ == "__main__":
    main()
