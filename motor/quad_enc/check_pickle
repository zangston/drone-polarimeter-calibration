import pickle
import matplotlib.pyplot as plt

f = open("/home/declan/RPI/readout/quad_enc/nerd_file2.pkl", "rb")           #change this!!!!!!!!!!!!!!!!!!!!!!!!
spacetime = pickle.load(f)          #{time : radians}
f.close()

loc = []
time = []

old_key = 0
old_val = 0
zero = next(iter(spacetime.keys()))
print("Starting time [millisec]: ", zero)
for key, val in spacetime.items():
    if val > 0.000001:
        if old_val != val and old_key != key:
            loc.append(val)
            seconds = key/1000.0                # millisec to sec 
            time.append(seconds-(zero/1000.0))
            old_key = key
            old_val = val
        else:
            continue
    else:
        continue


# print(spacetime)
# print(loc)
# print(time)
plt.figure()
plt.ylabel("Motor Position [Radians]")
plt.xlabel("Time [seconds]")
plt.scatter(time, loc)
plt.show()
