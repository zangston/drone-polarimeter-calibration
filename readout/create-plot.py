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
counts_per_wheel_rev_guess = 2400
timezone_offset_hours = 4
plot_types = ["one_pixel", "all_pixel_sum", "all_pixel_avg", "ROI_sum", "ROI_average", "ROI_median"]


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


def save_plot(x, y, xlabel, ylabel, title, log_y, outpath):
    plt.figure(figsize=(8, 5))
    plt.plot(x, y, 'o', markersize=4)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    if log_y:
        plt.yscale("log")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(outpath)
    plt.close()


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <fits_exposure_dir> <encoder_data.pkl>")
        sys.exit(1)

    fits_dir = sys.argv[1]
    encoder_pkl = sys.argv[2]
    fits_path = os.path.join(fits_dir, "processed", "fits")
    plot_base_dir = os.path.join(fits_dir, "plots")
    os.makedirs(plot_base_dir, exist_ok=True)
    for folder in plot_types:
        os.makedirs(os.path.join(plot_base_dir, folder), exist_ok=True)

    fits_files = sorted(glob(os.path.join(fits_path, "*.fits")))
    if not fits_files:
        print(f"No FITS files found in {fits_path}")
        sys.exit(1)

    encoder_ts_array, encoder_counts = load_encoder_data(encoder_pkl)
    first_data = fits.getdata(fits_files[0])
    y_max, x_max = np.unravel_index(np.argmax(first_data), first_data.shape)

    encoders = []
    angles = []
    vals = {k: [] for k in plot_types}

    for ffile in fits_files:
        hdr = fits.getheader(ffile)
        fits_time_str = hdr.get("DATE-OBS")
        if not fits_time_str:
            continue
        fits_dt = datetime.fromisoformat(fits_time_str.replace("Z", "+00:00")) + timedelta(hours=timezone_offset_hours)
        fits_ts = fits_dt.replace(tzinfo=timezone.utc).timestamp()

        encoder_val = find_closest_encoder_angle(fits_ts, encoder_ts_array, encoder_counts)
        if encoder_val is None:
            continue

        data = fits.getdata(ffile)
        roi = data[y_max-1:y_max+2, x_max-1:x_max+2]  # 3x3 ROI

        encoders.append(encoder_val)
        rel = encoder_val - encoder_counts[0]
        frac = (rel / counts_per_wheel_rev_guess) % 1
        angles.append(frac * 2 * np.pi)

        vals["one_pixel"].append(data[y_max, x_max])
        vals["all_pixel_sum"].append(np.sum(data))
        vals["all_pixel_avg"].append(np.mean(data))
        vals["ROI_sum"].append(np.sum(roi))
        vals["ROI_average"].append(np.mean(roi))
        vals["ROI_median"].append(np.median(roi))

    encoders = np.array(encoders)
    angles = np.array(angles)

    for k in plot_types:
        y = np.array(vals[k])
        save_plot(encoders, y, "Encoder Count", k.replace("_", " ").title(), f"{k.replace('_', ' ').title()} vs Encoder", log_y=("sum" in k), outpath=os.path.join(plot_base_dir, k, f"{k}_vs_encoder.png"))
        save_plot(angles, y, "Plate Angle (rad)", k.replace("_", " ").title(), f"{k.replace('_', ' ').title()} vs Plate Angle", log_y=("sum" in k), outpath=os.path.join(plot_base_dir, k, f"{k}_vs_angle.png"))

    print("All plots saved to:", plot_base_dir)


if __name__ == "__main__":
    main()