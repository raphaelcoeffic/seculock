import serial
import sys
import time
import binascii

ser = serial.Serial(sys.argv[1], baudrate=115200, timeout=0.50)

ser.write(b'W\n') # EEPROM Read command

for i in range(0, 1024): # 32 * 1024 = 32KB
    ser.write(b'0123456789ABCDEF                ')
    while ser.out_waiting:
        pass
    if i % 2 == 1:
        print(ser.readline())

for i in range(0, 1024): # 32 * 1024 = 32KB
    ser.write(b'0123456789ABCDEF ooOO00OOoo     ')
    while ser.out_waiting:
        pass
    if i % 2 == 1:
        print(ser.readline())

print(ser.readline())
ser.close()
