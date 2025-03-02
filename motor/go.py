import subprocess
import keyboard
import getpass  # Import secure password input

file_path = "/home/declan/RPI/"
spin_rate = "250"
command = f'sudo -S pigpiod'  # No direct password in the command

def stop():
    try:
        subprocess.run([file_path + "motor/scripts/disable"])
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")

keyboard.add_hotkey('q', stop)

try:
    # Prompt user for password securely
    sudo_password = getpass.getpass("Enter sudo password: ")
    
    # Run sudo command securely
    subprocess.run(f'echo {sudo_password} | {command}', shell=True, check=True)
    
    # Motor
    subprocess.run([file_path + "motor/scripts/enable"])
    subprocess.run([file_path + "motor/scripts/backward"])
    subprocess.run([file_path + "motor/scripts/step"] + [spin_rate], check=True)
    # Encoder
    subprocess.run([file_path + "readout/quad_enc/declan-python-pickle"])  # change so it asks for arguments
    keyboard.wait()
except (subprocess.CalledProcessError, KeyboardInterrupt) as e:
    print(f"Error: {e}")
finally:
    keyboard.remove_hotkey('q')