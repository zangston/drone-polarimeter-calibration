import time
import os

# Function to run the motor control script with the desired action and speed
def run_motor_control(action, speed=None):
    if speed:
        command = f"python motor_control.py {action} {speed}"
    else:
        command = f"python motor_control.py {action}"
    
    os.system(command)

# Main test function
def motor_test():
    # Enable the motor
    print("Enabling motor...")
    run_motor_control("enable")
    time.sleep(1)  # Give the motor time to enable

    # Spin forward at speed 1000
    print("Spinning forward...")
    run_motor_control("forward", 1000)
    time.sleep(3)  # Spin for 3 seconds

    # Stop the motor
    print("Stopping motor...")
    run_motor_control("stop")
    time.sleep(1)

    # Spin backward at speed 1000
    print("Spinning backward...")
    run_motor_control("backward", 1000)
    time.sleep(3)  # Spin for 3 seconds

    # Stop the motor
    print("Stopping motor...")
    run_motor_control("stop")
    time.sleep(1)

    # Disable the motor
    print("Disabling motor...")
    run_motor_control("disable")

if __name__ == "__main__":
    motor_test()