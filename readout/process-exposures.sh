#!/bin/bash

# Check for input directory
if [ -z "$1" ]; then
    echo "Usage: $0 <directory>"
    exit 1
fi

BASE_DIR="$1"
RAW_DIR="$BASE_DIR/raw"
PROCESSED_DIR="$BASE_DIR/processed"
PY_SCRIPT="/home/declan/RPI/zwo/read-bin.py"

# Check if required directories exist
if [ ! -d "$RAW_DIR" ]; then
    echo "Error: $RAW_DIR does not exist."
    exit 1
fi

# Process each .bin file in the raw directory
# Create output directories
FITS_DIR="$PROCESSED_DIR/fits"
GRAY_DIR="$PROCESSED_DIR/grayscale"
COLOR_DIR="$PROCESSED_DIR/color"
mkdir -p "$FITS_DIR" "$GRAY_DIR" "$COLOR_DIR"
for bin_file in "$RAW_DIR"/*.bin; do
    if [ -f "$bin_file" ]; then
        echo "Processing $bin_file"

        # Change to raw directory

        # Get just the filename, not full path
        bin_filename=$(basename "$bin_file")

        python3 "$PY_SCRIPT" "$bin_file"

        base_name=$(basename "$bin_filename" .bin)

        # Move output files to appropriate folders
        mv "$RAW_DIR/${base_name}.fits" "$FITS_DIR/"
        mv "$RAW_DIR/${base_name}_gray.png" "$GRAY_DIR/"
        mv "$RAW_DIR/${base_name}_color.png" "$COLOR_DIR/"
    fi
done

echo "Processing complete. Output organized in:"
echo "  FITS:   $FITS_DIR"
echo "  Gray:   $GRAY_DIR"
echo "  Color:  $COLOR_DIR"