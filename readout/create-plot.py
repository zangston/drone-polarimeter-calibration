#!/usr/bin/env python3

import sys
import os
from glob import glob
from astropy.io import fits
from datetime import datetime, timezone, timedelta
import pickle
import numpy as np
import matplotlib.pyplot as plt

def load_encoder_data(pkl_path):
    with open(pkl_path, "rb") as f:
        data = pickle.load(f)
    timestamps_ms = np.array(list(data.keys()))
    angles = np.array(list(data.values()))
    encoder_times = np.array([
        datetime.fromtimestamp(ts / 1000.0, tz=timezone.utc)
        for ts in timestamps_ms
    ])
    print("Encoder time range:")
    print("Start:", encoder_times[0])
    print("End:", encoder_times[-1])
    return encoder_times, angles

def find_closest_encoder_angle(fits_dt, encoder_times, encoder_angles):
    fits_ts = fits_dt.timestamp()
    encoder_ts_array = np.array([et.timestamp() for et in encoder_times])
    if fits_ts < encoder_ts_array[0] or fits_ts > encoder_ts_array[-1]:
        return None
    idx = np.searchsorted(encoder_ts_array, fits_ts)
    if idx == 0:
        closest_idx = 0
    elif idx == len(encoder_ts_array):
        closest_idx = len(encoder_ts_array) - 1
    else:
        before = encoder_ts_array[idx - 1]
        after = encoder_ts_array[idx]
        closest_idx = idx - 1 if abs(fits_ts - before) < abs(fits_ts - after) else idx
    return encoder_angles[closest_idx]

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <fits_exposure_dir> <encoder_data.pkl>")
        sys.exit(1)

    fits_dir = sys.argv[1]
    encoder_pkl = sys.argv[2]
    fits_path = os.path.join(fits_dir, "processed", "fits")

    print(f"Looking for FITS files in: {fits_path}")
    fits_files = sorted(glob(os.path.join(fits_path, "*.fits")))
    if not fits_files:
        print(f"No FITS files found in {fits_path}")
        sys.exit(1)
    print(f"Found {len(fits_files)} FITS files")

    encoder_times, encoder_angles = load_encoder_data(encoder_pkl)

    # Find brightest pixel in the first file
    first_data = fits.getdata(fits_files[0])
    max_val = np.max(first_data)
    y_max, x_max = np.unravel_index(np.argmax(first_data), first_data.shape)
    y_pix_plot = first_data.shape[0] - 1 - y_max  # Convert to matplotlib-style Y (origin='lower')

    print(f"Using brightest pixel at (x={x_max}, y={y_pix_plot}) with value {max_val}")

    fits_times = []
    pixel_vals = []
    encoder_angles_for_fits = []

    for i, ffile in enumerate(fits_files):
        hdr = fits.getheader(ffile)
        fits_time_str = hdr.get('DATE-OBS', None)
        if fits_time_str is None:
            print(f"No DATE-OBS in FITS header for {ffile}")
            continue
        fits_dt = datetime.fromisoformat(fits_time_str.replace('Z', '+00:00'))
        fits_dt = fits_dt + timedelta(hours=4)  # Local to UTC
        fits_dt = fits_dt.replace(tzinfo=timezone.utc)

        data = fits.getdata(ffile)
        pixel_val = data[y_max, x_max]

        angle = find_closest_encoder_angle(fits_dt, encoder_times, encoder_angles)
        if angle is None:
            print(f"Warning: {fits_dt.isoformat()} outside encoder range")
            continue

        print(f"Frame {i+1}: Time={fits_dt.isoformat()}, Pixel=({x_max},{y_pix_plot}), Value={pixel_val}, Angle={angle:.3f}")
        fits_times.append(fits_dt)
        pixel_vals.append(pixel_val)
        encoder_angles_for_fits.append(angle)

    if not fits_times:
        print("No matching frames to plot.")
        sys.exit(1)

    plt.figure(figsize=(12,6))
    plt.plot(encoder_angles_for_fits, pixel_vals, 'o', label='Pixel Intensity vs Encoder Angle')
    plt.xlabel('Encoder Angle (rad)')
    plt.ylabel('Pixel Value')
    plt.title('Brightest Pixel Intensity vs Encoder Angle')
    plt.legend()
    plt.tight_layout()

    save_path = os.path.join(fits_dir, "brightest_pixel_encoder_angle_plot.png")
    plt.savefig(save_path)
    print(f"Plot saved to {save_path}")

if __name__ == "__main__":
    main()