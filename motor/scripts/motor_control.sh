#!/bin/bash

# Function to run shell commands
run_command() {
    eval "$1"
}

# Check if pigpiod is running, if not, start it
check_pigpiod() {
    if ! pgrep pigpiod > /dev/null; then
        echo "pigpiod not running. Starting pigpiod with sudo..."
        sudo pigpiod
    else
        echo "pigpiod is running."
    fi
}

# Enable motor
enable() {
    run_command "pigs w 16 0"
    run_command "pigs w 17 0"
    run_command "pigs w 20 0"
    run_command "pigs w 12 1"
}

# Disable motor
disable() {
    run_command "pigs w 12 0"
}

# Set motor direction
set_direction() {
    case "$1" in
        forward) run_command "pigs w 13 0" ;;
        backward) run_command "pigs w 13 1" ;;
        *) echo "Invalid direction! Use 'forward' or 'backward'." && exit 1 ;;
    esac
}

# Spin motor at given speed
spin() {
    run_command "pigs hp 19 $1 5000"
}

# Stop motor
stop() {
    run_command "pigs w 19 0"
}

# Main function
main() {
    check_pigpiod

    if [[ $# -lt 1 ]]; then
        echo "Usage: $0 {enable|disable|forward|backward|stop} [speed]"
        exit 1
    fi

    action="$1"

    case "$action" in
        enable) enable ;;
        disable) disable ;;
        forward|backward)
            if [[ $# -ne 2 ]]; then
                echo "Please provide a speed for spinning the motor."
                exit 1
            fi
            set_direction "$action"
            spin "$2"
            ;;
        stop) stop ;;
        *)
            echo "Unknown action: $action"
            echo "Usage: $0 {enable|disable|forward|backward|stop} [speed]"
            exit 1
            ;;
    esac
}

# Run main function with script arguments
main "$@"