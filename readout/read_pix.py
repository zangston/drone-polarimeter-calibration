from astropy.io import fits
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
from datetime import datetime
import pytz
import pickle
import numpy as np
import os
from scipy.signal import argrelextrema

###             How to use this code            ###

# First run the motor and encoder and the camera taking pics all at the same time
# open two terminals one for kstars and one for setting up `go` file. Start go file first then start kstars taking images. 
# After all images taken use go terminal and press q to stop motor and save data then Ctrl+C to stop count


# run motor and encoder simultaneously with sudo python3 /RPI/motor/go
### (not recommended) run motor manually with ~/RPI/motor/scripts then sudo pigpiod ./enable ./backwards ./step ###
### (not recommended) run encoder manually with ~/RPI/readout/quad_enc/ ./declan-python-pickle
# Then data for encoder is in ~/RPI/readout/quad_enc/nerd_pickle#.pkl
# Run camera with kstars (type kstars into terminal and it will appear. Go to ekos by using Ctrl+k. Then configure camera and setup.
# Camera data is in ~/RPI/readout/storage/try#/Light/Light###.fits

# Now in this program use the box function to align the focus where you are grabbing a pixel
# then use the grab and plot function to graph the pixel intensity vs motor position
###                                             ###



# can grab a pix
def grab_and_plot(int_time, num_of_frames, try_num, offset=1, x_pix=1548, y_pix=1040):
    """ loop over multiple images at that one pixel to find value of that pixel """
    
    arb_list = []
    pixel_list = []
    pixel_list_no_average = []
    sec_list = []
    avg_pix_list = []
    for i in range(num_of_frames):
        num = str(i+offset)
        if i+offset < 10:
            hdu1 = fits.open("/home/declan/RPI/readout/storage/try" + str(try_num) + "/Light/Light_00" + num + ".fits")
        elif i+offset < 100:
            hdu1 = fits.open("/home/declan/RPI/readout/storage/try" + str(try_num) + "/Light/Light_0" + num + ".fits")
        else:
            hdu1 = fits.open("/home/declan/RPI/readout/storage/try" + str(try_num) + "/Light/Light_" + num + ".fits")

        #average pixel value for noise subtraction
        data = hdu1[0].data #2D numpy array if single channel (I think it is)
        avg_pix = np.mean(data)
        avg_pix_list.append(avg_pix)        #for checking values
        
        #pixel value
        data = hdu1[0].data #assuming image is in the primary HDU 
        pixel_value = data[y_pix, x_pix] #top left is usually (0,0)
        pixel_list.append(pixel_value-avg_pix)
        pixel_list_no_average.append(pixel_value)

        #time stamp
        header = hdu1[0].header
        timestamp = header['DATE-OBS']
        dt = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S.%f')
        dt = dt.replace(tzinfo=pytz.UTC)  #turn from UTC to timezone aware
        seconds = dt.timestamp()
        sec_list.append(seconds)

        #create arbitrary index list
        arb_list.append(i) 

    #compare to spacetime
    f = open("/home/declan/RPI/motor/nerd_file16.pkl", "rb")	#change this!!!!!!!!!!!!!!!!!!!!!!!!!!!
    spacetime = pickle.load(f)          #{time:angle}
    f.close()

    ##################################
    #Camera lists
    rad_list = []
    pixel_new = []
    if len(pixel_list) != len(sec_list):
        print("camera pixel and sec lists don't match in size")
        return -1

    #Encoder
    angle = []
    t_enc_list = []
    for j in list(spacetime.keys()):
        t_enc_list.append(j/1000)

    #Numpy
    tolerance = 0.04
    arr1 = np.array(sec_list)
    arr2 = np.array(t_enc_list)

    abs_dif = np.abs(arr1[:, np.newaxis] - arr2)
    similar = np.where(abs_dif <= tolerance)
    
    for i in range(len(similar[0])):
        ind1 = similar[0][i]
        ind2 = similar[1][i]
        print("Index: ", ind1, ind2, "pixel: ", pixel_list[ind1], "angle: ", list(spacetime.values())[ind2])
        pixel_new.append(pixel_list[ind1])                  #more or less a useless line but good if wanting to compare len of pixel_new and angle for sanity check
        
        # get rid of duplicates
        if i > 0:
            if similar[0][i] != similar[0][i-1]:
                angle.append(list(spacetime.values())[ind2])
        else:
            angle.append(list(spacetime.values())[ind2])

    ##################################
    #Remapping small wheel rev to big wheel rev
    # ONLY WORKS IF MOTOR IS SPINNING CORRECT DIRECTION (backwards)     in RPI/motor/go it is already set to go backwards
    counter = 0
    prev_val = -1
    for i in angle:
        if i < prev_val:       # this inequality heavily depends on the direction of the motor (forwards > ; backwards <)
            counter += 1
            if counter > 5:
                counter = 0
        encoder_count = (i + (counter*380))
        rad_list.append(encoder_count)
        #rad_list.append(encoder_count * 2*np.pi/399 ) 
        # print("prev_val: ", prev_val, "i: ", i, "encoder count: ", encoder_count, "counter: ", counter)
        prev_val = i

    ######Chronological_color#########
    #pixel_new = np.array(pixel_new)
    #indices = argrelextrema(pixel_new, np.greater)
    normalized_time = (sec_list - np.min(sec_list)) / (np.max(sec_list)-np.min(sec_list))
    colormap = plt.cm.viridis
    colors = colormap(normalized_time)

    ######plot#######################
    #plt.scatter(arb_list, pixel_list)
    #plt.scatter(arb_list, pixel_list_no_average)
    print(len(rad_list), len(pixel_list))
    plt.scatter(rad_list, pixel_list)
    #plt.scatter(rad_list, pixel_new, color=colors)
    #plt.plot(rad_list, pixel_new)

    ##########colorbar################
#    sm = plt.cm.ScalarMappable(cmap=colormap, norm=plt.Normalize(vmin=np.min(sec_list), vmax=np.max(sec_list)))
#    plt.colorbar(sm, label='Timestamp')

    ###########Pretty-Graph###########
    title = str(int_time) + 'int_time;' + str(num_of_frames) + "num_of_frames"
    plt.title(title)
    plt.ylabel('Pixel value [e- count]')
    plt.xlabel("Motor Angle [radians]")
    #plt.savefig("/home/declan/RPI/readout/graphs/" + str(title) + "trynum" + str(try_num) + "seconds" + ".jpeg")
    plt.show()
    
    ###########Cleanup################
    hdu1.close()

    return rad_list, pixel_list


def box(try_num, width, x_pix=1548, y_pix=1040):
    #open the image
    hdu1 = fits.open("/home/declan/RPI/readout/storage/try" + str(try_num) + "/Light/Light_001.fits")
    image_data = hdu1[0].data
    hdu1.close()

    #change the value to find the box
    image_data[y_pix-width:y_pix+width, x_pix-width:x_pix+width] = 0
    plt.imshow(image_data, cmap="gray", norm=LogNorm())
    plt.colorbar()
    plt.show()


def avg_plot():
    pass
# idea to take in a "box" of pixels and average over them. 
# Helps minimize noise if drone moves


def read2file(try_num, list1, list2):
    filename = f"/home/declan/RPI/readout/storage/output/output_{try_num}"
    if os.path.exists(filename):
        os.remove(filename)
    with open(filename, "w") as f:
        for item in list1:
            f.write(f"{item}\n")
        f.write("\n")
        for item in list2:
            f.write(f"{item}\n")

    
##########################DO STUFF##############################
box(25, 20, 1520, 1220)                         # box(try_num, width, x_pix=1548, y_pix=1040)
#list1, list2 = grab_and_plot(0.05, 250, 26, 1, 1520, 1220)    # grab_and_plot(int_time, num_of_frames, try_num, offset=1, x_pix=1548, y_pix=1040):
#read2file(26, list1, list2)                                    # read2file(try_num, list1, list2)
#### try using color map based on time from chat gpt
