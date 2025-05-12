import matplotlib.pyplot as plt
from astropy.io import fits
import numpy as np
import sys
import os

avg_pix_list = []
pix_val_list = []
timestamp_list = []

# Get directory from command line
if len(sys.argv) < 2:
    print("Usage: python script.py <path_to_fits_directory>")
    sys.exit(1)

base_dir = sys.argv[1]
fits_dir = os.path.join(base_dir, "processed", "fits")
timestamp_str = os.path.basename(base_dir).replace("exposures-", "")

# Prompt for pixel location
x_pix = int(input("Enter x pixel value: "))
y_pix = int(input("Enter y pixel value: "))

def grab_and_plot():
    global avg_pix_list, pix_val_list, timestamp_list

    fits_files = sorted(f for f in os.listdir(fits_dir) if f.endswith(".fits"))

    for i, filename in enumerate(fits_files, 1):
        hdu1 = fits.open(os.path.join(fits_dir, filename))

        # average pixel value for noise subtraction
        data = hdu1['GREEN1'].data
        avg_pix = np.mean(data)
        avg_pix_list.append(avg_pix)

        # pixel value
        pixel_value = data[y_pix, x_pix]
        pix_val_list.append(pixel_value)

        # time stamp
        header = hdu1[0].header  # Primary HDU
        timestamp = header['DATE-OBS']
        timestamp_list.append(timestamp)

        hdu1.close()

        print(f"Image {i} - Time: {timestamp}, Pixel Value: {pixel_value}, Avg: {avg_pix}")

def box():
    first_file = sorted(f for f in os.listdir(fits_dir) if f.endswith(".fits"))[0]
    hdu1 = fits.open(os.path.join(fits_dir, first_file))
    image_data = hdu1['GREEN1'].data
    hdu1.close()

    plt.imshow(image_data, cmap='gray')
    plt.scatter(x_pix, y_pix, s=40, edgecolor='red', facecolor='none')
    plt.title("Pixel Location")
    plt.show()

box()
grab_and_plot()

plt.figure(figsize=(10, 6))
plt.scatter(timestamp_list, pix_val_list, label='Pixel Value')
plt.scatter(timestamp_list, avg_pix_list, label='Average Pixel Value', marker='x')
plt.xticks(rotation=45)
plt.xlabel("Timestamp")
plt.ylabel("Pixel Values")
plt.title("Pixel and Average Pixel Values Over Time")
plt.legend()
plt.tight_layout()
output_name = os.path.join(base_dir, f"pixel_plot_{timestamp_str}.png")
plt.savefig(output_name)
print(f"Output diagram saved to {output_name}")
plt.show()