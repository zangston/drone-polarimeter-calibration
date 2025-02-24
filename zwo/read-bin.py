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

    # Extract the individual color channels
    red_channel = image_data[::2, ::2]  # Assuming red is in odd rows and columns (example, adjust based on your data layout)
    green1_channel = image_data[::2, 1::2]  # Assuming green1 is in alternating rows
    green2_channel = image_data[1::2, ::2]  # Assuming green2 is in alternating columns
    blue_channel = image_data[1::2, 1::2]  # Assuming blue is in even rows and columns

    # Create a FITS file with multiple extensions
    fits_file = os.path.splitext(binary_file)[0] + ".fits"
    hdul = fits.HDUList()

    # Add the original grayscale image as the first extension
    hdu_gray = fits.PrimaryHDU(image_data)
    hdul.append(hdu_gray)

    # Add individual color channels as extensions
    hdul.append(fits.ImageHDU(red_channel, name='RED'))
    hdul.append(fits.ImageHDU(green1_channel, name='GREEN1'))
    hdul.append(fits.ImageHDU(green2_channel, name='GREEN2'))
    hdul.append(fits.ImageHDU(blue_channel, name='BLUE'))

    # Create the full-color composite image (stack the channels)
    color_image = np.stack([red_channel, green1_channel, blue_channel], axis=-1)
    hdul.append(fits.ImageHDU(color_image, name='COLOR_COMPOSITE'))

    # Save all data to the FITS file
    hdul.writeto(fits_file, overwrite=True)
    print(f"Saved all data to FITS file: {fits_file}")

    # Save image as PNG
    png_file = os.path.splitext(binary_file)[0] + "_color.png"
    plt.imsave(png_file, color_image)
    print(f"Saved Color PNG image: {png_file}")

    # Save grayscale image as PNG
    png_file_gray = os.path.splitext(binary_file)[0] + "_gray.png"
    plt.imsave(png_file_gray, image_data, cmap="gray")
    print(f"Saved Grayscale PNG image: {png_file_gray}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python convert_bin_to_fits.py <input.bin>")
        sys.exit(1)

    binary_to_fits(sys.argv[1])