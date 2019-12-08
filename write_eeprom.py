#!/usr/bin/env python

import serial
import sys
import time
import binascii

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix), end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()

ser = serial.Serial(sys.argv[1], baudrate=115200, timeout=0.5)
inf = open(sys.argv[2], 'rb')

ser.write(b'W\n') # EEPROM Write command

i = 0
while True:

    b = inf.read(32)
    if len(b) == 0:
        break
    
    ser.write(b)
    ser.flush()

    if i % 2 == 1:
        line = ser.readline().decode('ascii')
        if line[0] == '#':
            print('\nErrory detected!')
            break
    i = i + 1

    printProgressBar(i, 2048, suffix='uploaded', length=50)

print(ser.readline().decode('ascii'))
ser.close()
