#!/bin/bash

# Configuration
NUM_EXPOSURES=10         # Number of exposures to take
STEP_DELAY=1             # Time (seconds) between motor stop and exposure
CSV_FILE="motor_exposure_log.csv"  # CSV file to store motor angles and filenames

# Path to external programs
MOTOR_SCRIPT="/path/to/motor_control_script.py"  # Replace with actual path
EXPOSURE_CMD="/path/to/exposure_program"         # Replace with actual exposure command
ENCODER_CMD="/path/to/encoder_readout"           # Replace with actual command to read encoder angle

# Ensure CSV file has a header
echo "Exposure_File,Motor_Angle" > "$CSV_FILE"

for i in $(seq 1 $NUM_EXPOSURES); do
    echo "Starting exposure $i of $NUM_EXPOSURES"

    # Step the motor (modify this if a different command is needed)
    python3 "$MOTOR_SCRIPT" --step  # Replace with actual step command if needed

    # Wait for motor to stabilize before taking an exposure
    sleep "$STEP_DELAY"

    # Read motor angle from encoder
    MOTOR_ANGLE=$($ENCODER_CMD)  # Modify this to match encoder output parsing

    # Generate exposure filename
    EXPOSURE_FILE="exposure_${i}.bin"

    # Take exposure
    $EXPOSURE_CMD --output "$EXPOSURE_FILE"

    # Log motor angle and exposure filename
    echo "$EXPOSURE_FILE,$MOTOR_ANGLE" >> "$CSV_FILE"

    echo "Captured $EXPOSURE_FILE at motor angle $MOTOR_ANGLE"
done

echo "All exposures captured. Data saved to $CSV_FILE."