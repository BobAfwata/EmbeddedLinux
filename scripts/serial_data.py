import serial
from time import sleep

port = "/dev/ttyACM0"
ser = serial.Serial(port, 115200, timeout=0)

while True:
    data = ser.read(9999)
    if len(data) > 0:
        print ' ', data

    sleep(0.5)
    print 'Serial still available ..'

ser.close()