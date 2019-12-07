import serial
import sys
import time
import binascii

ser = serial.Serial(sys.argv[1], timeout=0.2)

ser.write(b'R\n') # EEPROM Read command

# Read 64KB
for i in range(0, 1024):
    sys.stdout.buffer.write(ser.read(64))

ser.close()
