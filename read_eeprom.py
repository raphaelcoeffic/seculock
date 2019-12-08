#!/usr/bin/env python

import serial
import sys
import time

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix), end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()


ser = serial.Serial(sys.argv[1], timeout=0.2)
outf = open(sys.argv[2], 'wb')

ser.write(b'R\n') # EEPROM Read command

# Read 64KB
for i in range(0, 1024):
    outf.write(ser.read(64))
    printProgressBar(i, 1023, suffix='downloaded', length=50, fill='#')

ser.close()
