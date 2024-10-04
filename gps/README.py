"""
In order to get the gps to run you must:
Using serial port ttyS0 which is under /dev/serial0 it should be all set up

1) gpsmon -more data points
2) cgps -tells how long and if you have gotten a fix
3) minicom -b 9600 -o -D /dev/serial0
	ctrl+a+q to quit
	ctrl+a+o for settings
	This brings up the NMEA sentences basically the raw data for gps

"""
