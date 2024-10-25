import os
import sys

# Function to run shell commands
def run_command(command):
    os.system(command)

# Check if pigpiod is running, if not, start it
def check_pigpiod():
    status = os.system("pgrep pigpiod > /dev/null")
    if status != 0:
        print("pigpiod not running. Starting pigpiod with sudo...")
        os.system("sudo pigpiod")
    else:
        print("pigpiod is running.")

# Enable motor
def enable():
    run_command("pigs w 16 0")
    run_command("pigs w 17 0")
    run_command("pigs w 20 0")
    run_command("pigs w 12 1")

# Disable motor
def disable():
    run_command("pigs w 12 0")

# Set motor direction (forward or backward)
def set_direction(direction):
    if direction == "forward":
        run_command("pigs w 13 0")
    elif direction == "backward":
        run_command("pigs w 13 1")
    else:
        print("Invalid direction! Use 'forward' or 'backward'.")
        sys.exit(1)

# Spin motor at a given speed
def spin(speed):
    run_command(f"pigs hp 19 {speed} 5000")

# Stop motor
def stop():
    run_command("pigs w 19 0")

# Main function to control motor based on user input
def main():
    # Check if pigpiod is running, and start it if necessary
    check_pigpiod()

    if len(sys.argv) < 2:
        print("Usage: python motor_control.py {enable|disable|forward|backward|stop} [speed]")
        sys.exit(1)

    action = sys.argv[1]

    if action == "enable":
        enable()
    elif action == "disable":
        disable()
    elif action == "forward" or action == "backward":
        if len(sys.argv) != 3:
            print("Please provide a speed for spinning the motor.")
            sys.exit(1)
        speed = sys.argv[2]
        set_direction(action)
        spin(speed)
    elif action == "stop":
        stop()
    else:
        print(f"Unknown action: {action}")
        print("Usage: python motor_control.py {enable|disable|forward|backward|stop} [speed]")
        sys.exit(1)

if __name__ == "__main__":
    main()