import serial.tools.list_ports
import serial
import time
import sys
import waitforkey as waitforkey

def send_qsb_command(device, command):
    if command[-1] != '\n':
        command += "\n"
        
    return device.write(command.encode('ascii')) > 0
       
def read_qsb_response(device):
    
    return (device.read_until(size=21))    

def process_qsb_command(device, command):
    if send_qsb_command(device, command):
        parse_qsb_respose(read_qsb_response(device))
    
def parse_qsb_respose(response_bytes):
    if len(response_bytes) >= 21:        
        response = response_bytes.decode('ascii').rstrip()
        cmdType = response[0]
        if 'RWS'.find(cmdType) and response[-1] == '!' :
            register = response[1:3] 
            data = int(response[3:11],16)
            timestamp = int(response[11:19],16) * 1.95
            print(f'Encoder, {cmdType}, {register}, {data}, {round(timestamp,2)}', sep=' ', end='\r', flush=True)
        else:
            print(f'Read Error, {0}, {0}, {0}, {0}', sep=' ', end='\r', flush=True)

def display_qsb_count_response(response_bytes):
    if len(response_bytes) >= 21:
        try:
            response = response_bytes.decode('ascii').rstrip()
            cmdType = response[0]
            if 'RS'.find(cmdType) and response[-1] == '!' :
                data = int(response[3:11],16)
                timestamp = int(response[11:19],16) * 1.95
                print(f'Count = {data}, QSB TimeStamp = {round(timestamp,2)}', sep=' ', end='\r', flush=True)
            else:
                print(f'Read Error: {response}\n')
        except Exception as ex:
            print('Parse exeption: {}', ex.__cause__)
            pass #
    
serialPort = 'COM7'

if (len(sys.argv) > 1):
    serialPort = sys.argv[1]
else:
    all_comports = serial.tools.list_ports.comports()
    all_comports.sort()
    num = 0
    ports = [] 
    for comport in all_comports:
        ports.append(comport.device)
    
    if len(ports) == 1:
        serialPort = ports[0]
    else:
        print ('Missing port parameter')
        print ('syntax: streamQSB.py port')
        print (f'Available ports: {", ".join(ports)}')    
        exit(0)

# Create a serial object for the QSB encoder
serEncoder = serial.Serial(
    port=serialPort, 
    baudrate=230400,
    bytesize=serial.EIGHTBITS, # 8 data bits
    parity=serial.PARITY_NONE, # no parity
    stopbits=serial.STOPBITS_ONE, # 1 stop b
    timeout=1    
    )

try:
    # Check if the port is already open
    if not serEncoder.is_open:
        # Open the serial port
        serEncoder.open()        
    
    # Allow time for communications to stabilize
    time.sleep(2)       
    
    # This will read the QSB-TYPE REV: example: b'QSB-S  0E!\r\n'
    # or timeout after 1 second.
    print(serEncoder.readline())
    
    # Configure the QSB response format to include line feed and a time stamp
    process_qsb_command(serEncoder, 'W155')
    
    # Reset the timestamp
    process_qsb_command(serEncoder, 'W0D1')    

    # Set the encoder mode to quadrature
    process_qsb_command(serEncoder, 'W000')
    
    # Set the quadrature mode: X4, modulo-N, index disabled
    process_qsb_command(serEncoder, 'W03F')
    
    # Set the counter mode: Enabled, Non-invert, No-triggers
    process_qsb_command(serEncoder, 'W040')

    # Set the maximum count (Preset) to decimal 499
    process_qsb_command(serEncoder, "W081F3")

    # Reset the counter to zero
    process_qsb_command(serEncoder, 'W092')
    
    # Reset the timestamp
    process_qsb_command(serEncoder, 'W0D1')

    # Set the threshold register to 1 to stream when count value changes by 1.
    # If the count value is to be streamed even when the count value does not 
    # change, then write a value of 0 ('W0B0'). 
    process_qsb_command(serEncoder, 'W0B1')
    
    # Set the interval rate register to zero for the fastest rate possible 
    process_qsb_command(serEncoder, 'W0C0')
        
    # Begin streaming the encoder count register 
    process_qsb_command(serEncoder, 'S0E')
    
    # Clear the screen.
    #os.system('cls')
    kb = waitforkey.KBHit()
    print("Press ESC to stop streaming and exit")

    # loop until a ESC is pressed.
    while True:
        if kb.kbhit():
            c = kb.getch()
            if ord(c) == 27: # ESC
                break
        
        display_qsb_count_response(read_qsb_response(serEncoder))

    print ("\nStreaming stopped.\n")
    # Reading the encoder counts register will stop streaming encoder counts
    process_qsb_command(serEncoder, 'R0E')
    
       
except serial.SerialException as se:
    print(f"SerialException: {se}")
    if serEncoder.is_open:
        serEncoder.close()
