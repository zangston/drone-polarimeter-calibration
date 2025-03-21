#!/bin/bash

# Configuration
NUM_EXPOSURES=10         # Number of exposures to take
STEP_DELAY=1             # Time (seconds) between motor stop and exposure
CSV_FILE="motor_exposure_log.csv"  # CSV file to store motor angles and filenames

# Path to external programs
MOTOR_SCRIPT="/home/declan/RPI/motor/scripts/motor_control.sh"
EXPOSURE_CMD="/home/declan/RPI/zwo/capture-exposure.out"
ENCODER_CMD="/home/declan/RPI/readout/quad_enc/declan-python-pickle"

# Ensure CSV file has a header
echo "Exposure_File,Motor_Angle" > "$CSV_FILE"

for i in $(seq 1 $NUM_EXPOSURES); do
    echo "Starting exposure $i of $NUM_EXPOSURES"

    # Step the motor (now supported in motor_control.sh)
    "$MOTOR_SCRIPT" step  # Optional: add speed argument if needed

    # Wait for motor to stabilize before taking an exposure
    sleep "$STEP_DELAY"

    # Read motor angle from encoder
    MOTOR_ANGLE=$("$ENCODER_CMD")  # Modify this if encoder output needs parsing

    # Generate exposure filename
    EXPOSURE_FILE="exposure_${i}.bin"

    # Take exposure
    "$EXPOSURE_CMD" --output "$EXPOSURE_FILE"

    # Log motor angle and exposure filename
    echo "$EXPOSURE_FILE,$MOTOR_ANGLE" >> "$CSV_FILE"

    echo "Captured $EXPOSURE_FILE at motor angle $MOTOR_ANGLE"
done

echo "All exposures captured. Data saved to $CSV_FILE."