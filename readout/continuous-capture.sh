#!/bin/bash
# Create timestamped output directory with subfolders
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
BASE_DIR="exposures-$TIMESTAMP"
RAW_DIR="$BASE_DIR/raw"
PROCESSED_DIR="$BASE_DIR/processed"
mkdir -p "$RAW_DIR" "$PROCESSED_DIR"

# Full path to the exposure capture binary
CAPTURE_BIN="/home/declan/RPI/zwo/capture-exposure.out"

# Interval between exposures in seconds (1.5 seconds)
INTERVAL=1.5

# Trap keyboard interrupt (Ctrl+C) and exit cleanly
trap "echo -e '\nCapture interrupted by user. Exiting.'; exit 0" SIGINT

echo "Saving exposures to $RAW_DIR. Press Ctrl+C to stop."

cd "$RAW_DIR"
while true; do
    "$CAPTURE_BIN" 0.001 > "exposure-$(date +%Y%m%d-%H%M%S).bin"
    # Removed file count variable increment; file count is determined on trap
    sleep $INTERVAL
done