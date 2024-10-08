# How to use the polarimeter RPI
work in progress...

## Motor instructions
Motor files are found in 'RPI/motor/scripts'
 
1. Identify the encoder hat (should be the top of the stack with colored wires coming out of the green pin housing)
2. Flip the hat switch to ON 
3. Make sure the encoder (big black USB stick with colored wires)  is plugged into the blue USB 3.0 port of the RPI
4. From home directory, run $ cd RPI/motor/scripts
5. Enable pigpiod: $ sudo pigpiod; enter password
6. $ ./enable; ./forward; ./step [speed]

## Camera instructions

1. Connect camera
	1. Open KStars from applications list
	2. Ctrl + K in Kstars to open Ekos
	3. Select profile associated with camera (named ZWO...)
	4. Press play button to start Ekos to show camera controls
2. Click on camera icon in Ekos top bar to navigate to capture seccionts
3. Click play button on right side fo start capture
