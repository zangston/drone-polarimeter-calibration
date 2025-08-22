import pigpio
import matplotlib.pyplot as plt
import subprocess
from datetime import datetime
import pickle
import rotaryio
import board

# setup
pi = pigpio.pi()
# gpio pins
A_plus = 23
Z_plus = 24
B_plus = 27
# defining lists
counter_list = []
AB_list = []        #A and B together so each input is either 00, 01 , 10, or 11
AB_val_list = []
#define constants
step_size = 1.8/8    # 1.8/8


def encoder_data(A_plus, B_plus):
    pulse = 0
    AB_list.append(str(pi.read(A_plus))+str(pi.read(B_plus)))
    if len(AB_list) > 1:
        if AB_list[-1] != AB_list[-2]:
            #print(AB_list[-1], AB_list[-2])
            pulse = 1
    return pulse


def encoder_2():
    encoder = rotaryio.IncrementalEncoder(board.D23, board.D27)
    last_position = None
    while True:
        position = encoder.position
        if last_position is None or position != last_position:
            print(position)
        last_position = position


#this needs help! maybe get rid of the while loop and just have it time.sleep(#) until it is at a good position
#also not very important to have at all according to Brad
def go_to_zero():       
    while pi.read(Z_plus) != 1:
        if pi.read(Z_plus) == 1:
            subprocess.Popen("~/RPI/motor/scripts/stop", stdout=subprocess.PIPE, shell=True)
    return


#make sure not skipping steps cuz I think it is rn. Look at AB list output   QUADRATURE LIBRARY!!!!!!
try:
    """
    subprocess.Popen("~/RPI/motor/scripts/step 500", stdout=subprocess.PIPE, shell=True)
    go_to_zero()
    time.sleep(5)
    """
    max_step = 1600
    i = 0
    counter = 0
    spacetime = {}
    subprocess.Popen("~/RPI/motor/scripts/step 500", stdout=subprocess.PIPE, shell=True)
    while i < max_step*51: #6 = one full rev of big wheel (10 = about 50 pics at 0.01 expo time)
        if encoder_data(A_plus, B_plus) == 1:
            i += 1
            now = datetime.now()
            seconds = now.timestamp()
            loc = counter*step_size
            counter += 1
            if counter >= max_step:
                counter = 0
            spacetime[seconds] = loc

    #save as a dictionary {time:loc} and output to a file only change try number if want to keep old data
    with open("/home/declan/RPI/readout/spacetime/try3.pkl","wb") as output_file:          #change try number here!
        pickle.dump(spacetime, output_file)
        output_file.close()
    subprocess.Popen("~/RPI/motor/scripts/stop", stdout=subprocess.PIPE, shell=True)
    output_file = open("/home/declan/RPI/readout/spacetime/try3.pkl", "rb")                 #change here too ^^^^^^
    data = pickle.load(output_file) 
    print(data)
    output_file.close()
    pi.stop()
    
except KeyboardInterrupt:
    pass

pi.stop() 
