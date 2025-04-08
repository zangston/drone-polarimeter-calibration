#!/bin/bash

# Full path to the exposure capture binary
CAPTURE_BIN="/home/declan/RPI/zwo/capture-exposure.out"

# Interval between exposures in seconds (100 per second = 0.01s)
INTERVAL=0.01

# Trap keyboard interrupt (Ctrl+C) and exit cleanly
trap "echo -e '\nCapture interrupted by user. Exiting.'; exit 0" SIGINT

echo "Starting rapid capture loop (100 exposures/sec). Press Ctrl+C to stop."

while true; do
    "$CAPTURE_BIN" 0.001 &
    sleep $INTERVAL
done