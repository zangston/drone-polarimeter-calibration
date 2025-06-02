import matplotlib.pyplot as plt
from astropy.io import fits
import numpy as np
import sys
import os

pixel_sum_list = []
timestamp_list = []

# Get directory from command line
if len(sys.argv) < 2:
    print("Usage: python script.py <path_to_fits_directory>")
    sys.exit(1)

base_dir = sys.argv[1]
fits_dir = os.path.join(base_dir, "processed", "fits")
timestamp_str = os.path.basename(base_dir).replace("exposures-", "")

def grab_and_plot():
    global pixel_sum_list, timestamp_list

    fits_files = sorted(f for f in os.listdir(fits_dir) if f.endswith(".fits"))

    for i, filename in enumerate(fits_files, 1):
        hdu1 = fits.open(os.path.join(fits_dir, filename))

        data = hdu1['GREEN1'].data

        # Compute pixel sum
        pixel_sum = np.sum(data)
        pixel_sum_list.append(pixel_sum)

        # Time stamp
        header = hdu1[0].header  # Primary HDU
        timestamp = header['DATE-OBS']
        timestamp_list.append(timestamp)

        hdu1.close()

        print(f"Image {i} - Time: {timestamp}, Pixel Sum: {pixel_sum}")

grab_and_plot()

plt.figure(figsize=(10, 6))
plt.scatter(timestamp_list, pixel_sum_list, label='Pixel Sum')
plt.xticks(rotation=45)
plt.xlabel("Timestamp")
plt.ylabel("Total Pixel Value")
plt.title("Total Pixel Value Over Time")
plt.legend()
plt.tight_layout()
output_name = os.path.join(base_dir, f"pixel_plot_{timestamp_str}.png")
plt.savefig(output_name)
print(f"Output diagram saved to {output_name}")
plt.show()