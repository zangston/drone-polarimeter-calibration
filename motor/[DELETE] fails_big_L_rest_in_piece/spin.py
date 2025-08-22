import time
import lgpio


A1 = 17
B1 = 13
A2 = 11
B2 = 15

"""
enable = 12 # 1 high   0 low
dir = 13 # 0 forward   1 backward
step = 19
"""
sleepy_time = 0.1
steps = 20

# open gpiochip0 device
h = lgpio.gpiochip_open(0)

# set pins as outputs
"""
lgpio.gpio_claim_output(h,enable)
lgpio.gpio_claim_output(h,dir)
lgpio.gpio_claim_output(h,19)
"""
lgpio.gpio_claim_output(h,A1)
lgpio.gpio_claim_output(h,B1)
lgpio.gpio_claim_output(h,A2)
lgpio.gpio_claim_output(h,B2)

# write to pins
"""
lgpio.gpio_write(h,enable,1)
lgpio.gpio_write(h,dir,1)
lgpio.gpio_write(h,19,1)
time.sleep(3)
"""

def spin(steps, delay):
	for i in range(steps):
		# tried (1,0,1,0) => (0,1,0,1)
		# tried (1100) => (0110) => (0011) => (1001)
		# tried 
		lgpio.gpio_write(h,A1,1)  # (1,0,1,0) = (A1,B1,A2,B2)
		lgpio.gpio_write(h,A2,0)
		lgpio.gpio_write(h,B1,0)
		lgpio.gpio_write(h,B2,0)
		time.sleep(delay)

		lgpio.gpio_write(h,A1,1)  # (0,1,1,0)
		lgpio.gpio_write(h,A2,1)
		lgpio.gpio_write(h,B1,0)
		lgpio.gpio_write(h,B2,0)
		time.sleep(delay)

		lgpio.gpio_write(h,A1,0)  # (0,1,0,1)
		lgpio.gpio_write(h,A2,1)
		lgpio.gpio_write(h,B1,0)
		lgpio.gpio_write(h,B2,0)
		time.sleep(delay)

		lgpio.gpio_write(h,A1,0)  # (1,0,0,1)
		lgpio.gpio_write(h,A2,0)
		lgpio.gpio_write(h,B1,0)
		lgpio.gpio_write(h,B2,0)
		time.sleep(delay)
"""
		lgpio.gpio_write(h,A1,0)  # (1,0,1,0) = (A1,B1,A2,B2)
		lgpio.gpio_write(h,A2,0)
		lgpio.gpio_write(h,B1,1)
		lgpio.gpio_write(h,B2,0)
		time.sleep(delay)

		lgpio.gpio_write(h,A1,0)  # (0,1,1,0)
		lgpio.gpio_write(h,A2,0)
		lgpio.gpio_write(h,B1,1)
		lgpio.gpio_write(h,B2,1)
		time.sleep(delay)

		lgpio.gpio_write(h,A1,0)  # (0,1,0,1)
		lgpio.gpio_write(h,A2,0)
		lgpio.gpio_write(h,B1,0)
		lgpio.gpio_write(h,B2,1)
		time.sleep(delay)

		lgpio.gpio_write(h,A1,0)  # (1,0,0,1)
		lgpio.gpio_write(h,A2,0)
		lgpio.gpio_write(h,B1,0)
		lgpio.gpio_write(h,B2,0)
		time.sleep(delay)
"""
#		print(i)
#	lgpio.gpio_write(h,A1,0)
#	lgpio.gpio_write(h,B1,0)
#	lgpio.gpio_write(h,A2,0)
#	lgpio.gpio_write(h,B2,0)

"""
try:
	while True:
#		for i in range(0,steps):
#			print(i)
		lgpio.gpio_write(h,dir,1)
		time.sleep(sleepy_time)

#		for i in range(0,steps):
#			print("2", i)
		lgpio.gpio_write(h,dir,0)
		time.sleep(sleepy_time)

except KeyboardInterrupt:
	lgpio.gpio_write(h,enable,0)
"""
spin(steps,sleepy_time)
# lgpio.gpio_write(h,enable,0)
lgpio.gpiochip_close(h)
