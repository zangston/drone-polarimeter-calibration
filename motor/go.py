import subprocess
import keyboard
import os

file_path = "/home/declan/RPI/"
spin_rate = "250"

def stop():
    try:
        subprocess.run(["python3", file_path + "motor/scripts/motor_control.py", "stop"], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")

keyboard.add_hotkey('q', stop)

try:
    # Ensure pigpiod is running without requiring a password
    subprocess.run(["sudo", "pigpiod"], check=True)

    # Motor Control
    subprocess.run(["python3", file_path + "motor/scripts/motor_control.py", "enable"], check=True)
    subprocess.run(["python3", file_path + "motor/scripts/motor_control.py", "backward", spin_rate], check=True)

    # Encoder Readout
    subprocess.run([file_path + "readout/quad_enc/declan-python-pickle"])  # Ensure this script handles arguments if needed

    keyboard.wait()
except (subprocess.CalledProcessError, KeyboardInterrupt) as e:
    print(f"Error: {e}")
finally:
    keyboard.remove_hotkey('q')