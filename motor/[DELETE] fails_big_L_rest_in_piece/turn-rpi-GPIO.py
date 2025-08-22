import RPi.GPIO as GPIO
import time

#Define GPIO pins
A1 = 11
B1 = 12
A2 = 13
B2 = 15

#set mode
GPIO.setmode(GPIO.BOARD)

GPIO.setup(A1, GPIO.OUT)
GPIO.setup(B1, GPIO.OUT)
GPIO.setup(A2, GPIO.OUT)
GPIO.setup(B2, GPIO.OUT)


#spin func
def spin(steps, delay):
	for i in range(steps):
		GPIO.output(A1, True)
		GPIO.output(B1, False)
		GPIO.output(A2, True)
		GPIO.output(B2, False)
		time.sleep(delay)

		GPIO.output(A1, False)
		GPIO.output(A1, True)
		GPIO.output(A1, True)
		GPIO.output(A1, False)
		time.sleep(delay)

		GPIO.output(A1, False)
		GPIO.output(A1, True)
		GPIO.output(A1, False)
		GPIO.output(A1, True)
		time.sleep(delay)

		GPIO.output(A1, True)
		GPIO.output(A1, False)
		GPIO.output(A1, False)
		GPIO.output(A1, True)
		time.sleep(delay)

spin(100,0.1)

GPIO.cleanup()
