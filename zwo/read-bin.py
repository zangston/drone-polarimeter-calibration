import numpy as np
import matplotlib.pyplot as plt
from astropy.io import fits
import sys
import os

def binary_to_fits(binary_file):
    # Define camera dimensions
    width, height = 3096, 2080

    # Read binary file
    with open(binary_file, "rb") as f:
        pixel_data = np.frombuffer(f.read(), dtype=np.uint16).reshape((height, width))

    # Normalize data for visualization (scale 14-bit values to 8-bit)
    image_data = (pixel_data / pixel_data.max() * 255).astype(np.uint8)

    # Create FITS file
    fits_file = os.path.splitext(binary_file)[0] + ".fits"
    hdu = fits.PrimaryHDU(pixel_data)
    hdu.writeto(fits_file, overwrite=True)
    print(f"Saved FITS file: {fits_file}")

    # Save image as PNG
    png_file = os.path.splitext(binary_file)[0] + ".png"
    plt.imsave(png_file, image_data, cmap="gray")
    print(f"Saved PNG image: {png_file}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python convert_bin_to_fits.py <input.bin>")
        sys.exit(1)

    binary_to_fits(sys.argv[1])