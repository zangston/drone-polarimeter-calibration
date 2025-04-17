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

# Create output subdirectories
FITS_DIR="$PROCESSED_DIR/fits"
GRAY_DIR="$PROCESSED_DIR/gray"
COLOR_DIR="$PROCESSED_DIR/color"
mkdir -p "$FITS_DIR" "$GRAY_DIR" "$COLOR_DIR"

# Process each .bin file in the raw directory
for bin_file in "$RAW_DIR"/*.bin; do
    if [ -f "$bin_file" ]; then
        echo "Processing $bin_file"
        python3 "$PY_SCRIPT" "$bin_file"

        # Get base filename without extension
        base_name=$(basename "$bin_file" .bin)

        # Move output files to appropriate folders
        mv "${base_name}.FITS" "$FITS_DIR/" 2>/dev/null
        mv "${base_name}_gray.png" "$GRAY_DIR/" 2>/dev/null
        mv "${base_name}_color.png" "$COLOR_DIR/" 2>/dev/null
    fi
done

echo "Processing complete. Output organized in:"
echo "  FITS:   $FITS_DIR"
echo "  Gray:   $GRAY_DIR"
echo "  Color:  $COLOR_DIR"