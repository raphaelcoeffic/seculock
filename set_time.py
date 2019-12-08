#!/usr/bin/env python

import serial
import sys
import time

ser = serial.Serial(sys.argv[1], timeout=0.2)
ser.write(('T' + time.strftime('%s') + '\n').encode('ascii'))


