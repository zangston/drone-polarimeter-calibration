#!/usr/bin/env python3

import sys
import os
from glob import glob
from astropy.io import fits
from datetime import datetime, timezone, timedelta
import pickle
import numpy as np
import matplotlib.pyplot as plt

# === CONFIG ===
counts_per_wheel_rev_guess = 2410  # Adjust as needed
timezone_offset_hours = 4          # Local to UTC

def load_encoder_data(pkl_path):
    with open(pkl_path, "rb") as f:
        data = pickle.load(f)
    encoder_times_ms = np.array(list(data.keys()))
    encoder_counts = np.array(list(data.values()))
    encoder_times = np.array([
        datetime.fromtimestamp(ts / 1000.0, tz=timezone.utc)
        for ts in encoder_times_ms
    ])
    encoder_ts_float = np.array([et.timestamp() for et in encoder_times])
    return encoder_ts_float, encoder_counts

def find_closest_encoder_angle(fits_ts, encoder_ts_array, encoder_counts):
    if fits_ts < encoder_ts_array[0] or fits_ts > encoder_ts_array[-1]:
        return None
    idx = np.searchsorted(encoder_ts_array, fits_ts)
    if idx == 0:
        return encoder_counts[0]
    elif idx == len(encoder_ts_array):
        return encoder_counts[-1]
    before = encoder_ts_array[idx - 1]
    after = encoder_ts_array[idx]
    closest_idx = idx - 1 if abs(fits_ts - before) < abs(fits_ts - after) else idx
    return encoder_counts[closest_idx]

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <fits_exposure_dir> <encoder_data.pkl>")
        sys.exit(1)

    fits_dir = sys.argv[1]
    encoder_pkl = sys.argv[2]
    fits_path = os.path.join(fits_dir, "processed", "fits")

    fits_files = sorted(glob(os.path.join(fits_path, "*.fits")))
    if not fits_files:
        print(f"No FITS files found in {fits_path}")
        sys.exit(1)

    encoder_ts_array, encoder_counts = load_encoder_data(encoder_pkl)

    # === Find brightest pixel in first frame ===
    first_data = fits.getdata(fits_files[0])
    y_max, x_max = np.unravel_index(np.argmax(first_data), first_data.shape)
    print(f"Brightest pixel in first frame: (x={x_max}, y={first_data.shape[0] - 1 - y_max})")

    # === Storage ===
    pixel_sums = []
    pixel_vals = []
    encoder_vals = []

    for i, ffile in enumerate(fits_files):
        hdr = fits.getheader(ffile)
        fits_time_str = hdr.get("DATE-OBS")
        if not fits_time_str:
            continue

        fits_dt = datetime.fromisoformat(fits_time_str.replace("Z", "+00:00"))
        fits_dt += timedelta(hours=timezone_offset_hours)
        fits_dt = fits_dt.replace(tzinfo=timezone.utc)
        fits_ts = fits_dt.timestamp()

        encoder_val = find_closest_encoder_angle(fits_ts, encoder_ts_array, encoder_counts)
        if encoder_val is None:
            print(f"Skipping {ffile}: no encoder match")
            continue

        data = fits.getdata(ffile)
        pixel_sum = np.sum(data)
        pixel_val = data[y_max, x_max]

        pixel_sums.append(pixel_sum)
        pixel_vals.append(pixel_val)
        encoder_vals.append(encoder_val)

    # === Convert to numpy arrays ===
    pixel_sums = np.array(pixel_sums)
    pixel_vals = np.array(pixel_vals)
    encoder_vals = np.array(encoder_vals)

    # === Compute folded angles ===
    rel_counts = encoder_vals - encoder_vals[0]
    wheel_frac = (rel_counts / counts_per_wheel_rev_guess) % 1
    wheel_angles = wheel_frac * 2 * np.pi

    # === Plot 1: Pixel sum vs encoder ===
    plt.figure(figsize=(8, 5))
    plt.plot(encoder_vals, pixel_sums, 'o', label="Pixel Sum vs Encoder")
    plt.xlabel("Encoder Count")
    plt.ylabel("Pixel Sum")
    plt.yscale("log")
    plt.title("Pixel Sum vs Encoder Count")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(fits_dir, "plot_1_pixel_sum_vs_encoder.png"))

    # === Plot 2: Pixel sum vs plate angle (folded, log scale) ===
    plt.figure(figsize=(8, 5))
    plt.scatter(wheel_angles, pixel_sums, s=15, alpha=0.7)
    plt.xlabel("Plate Angle (radians)")
    plt.ylabel("Pixel Sum (log scale)")
    plt.yscale("log")
    plt.title("Pixel Sum vs Plate Angle (Folded, Log Scale)")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(fits_dir, "plot_2_pixel_sum_vs_angle_log.png"))

    # === Plot 3: Brightest pixel vs encoder ===
    plt.figure(figsize=(8, 5))
    plt.plot(encoder_vals, pixel_vals, 'o', label="Brightest Pixel vs Encoder")
    plt.xlabel("Encoder Count")
    plt.ylabel("Pixel Value")
    plt.title("Brightest Pixel vs Encoder Count")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(fits_dir, "plot_3_brightest_pixel_vs_encoder.png"))

    # === Plot 4: Brightest pixel vs plate angle (folded, linear) ===
    plt.figure(figsize=(8, 5))
    plt.scatter(wheel_angles, pixel_vals, s=15, alpha=0.7)
    plt.xlabel("Plate Angle (radians)")
    plt.ylabel("Brightest Pixel Value")
    plt.title("Brightest Pixel vs Plate Angle (Folded)")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(fits_dir, "plot_4_brightest_pixel_vs_angle_linear.png"))

    print("Saved 4 plots to", fits_dir)

if __name__ == "__main__":
    main()