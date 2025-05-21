import numpy as np
import matplotlib.pyplot as plt
from astropy.io import fits
import sys
import os
from datetime import datetime

def extract_timestamp_from_filename(filename):
    """
    Extracts timestamp from filenames like:
    exposure-YYYYMMDD-HHMMSS-mmm.bin
    """
    base = os.path.basename(filename)
    name, _ = os.path.splitext(base)

    try:
        # Expecting something like 'exposure-20250504-224836-153'
        parts = name.split("-")
        if len(parts) < 4:
            raise ValueError("Filename format incorrect for timestamp extraction")

        date_str = parts[1]
        time_str = parts[2]
        ms_str = parts[3]

        dt = datetime.strptime(date_str + time_str, "%Y%m%d%H%M%S")
        dt = dt.replace(microsecond=int(ms_str) * 1000)  # Convert milliseconds to microseconds
        return dt
    except Exception as e:
        print(f"Failed to parse timestamp from filename '{filename}': {e}")
        return None

def binary_to_fits(binary_file):
    # Define camera dimensions
    width, height = 3096, 2080

    # Read binary file
    with open(binary_file, "rb") as f:
        pixel_data = np.frombuffer(f.read(), dtype=np.uint16).reshape((height, width))

    # Normalize for 8-bit preview
    image_data = (pixel_data / pixel_data.max() * 255).astype(np.uint8)

    # Extract the individual color channels (adjust indexing if needed)
    red_channel = image_data[::2, ::2]
    green1_channel = image_data[::2, 1::2]
    green2_channel = image_data[1::2, ::2]
    blue_channel = image_data[1::2, 1::2]

    # Extract timestamp from filename
    timestamp = extract_timestamp_from_filename(binary_file)
    timestamp_str = timestamp.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z" if timestamp else "UNKNOWN"

    # Build FITS HDU list
    hdul = fits.HDUList()

    hdu_gray = fits.PrimaryHDU(image_data)
    hdu_gray.header['DATE-OBS'] = timestamp_str  # UTC timestamp with milliseconds
    hdul.append(hdu_gray)

    # Add color channels
    hdul.append(fits.ImageHDU(red_channel, name='RED'))
    hdul.append(fits.ImageHDU(green1_channel, name='GREEN1'))
    hdul.append(fits.ImageHDU(green2_channel, name='GREEN2'))
    hdul.append(fits.ImageHDU(blue_channel, name='BLUE'))

    # Composite image
    color_image = np.stack([red_channel, green1_channel, blue_channel], axis=-1)
    hdul.append(fits.ImageHDU(color_image, name='COLOR_COMPOSITE'))

    # Write FITS file
    fits_file = os.path.splitext(binary_file)[0] + ".fits"
    hdul.writeto(fits_file, overwrite=True)
    print(f"Saved FITS file with timestamp header: {fits_file}")

    # Save previews
    plt.imsave(fits_file.replace(".fits", "_color.png"), color_image)
    plt.imsave(fits_file.replace(".fits", "_gray.png"), image_data, cmap="gray")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python convert_bin_to_fits.py <input.bin>")
        sys.exit(1)

    binary_to_fits(sys.argv[1])