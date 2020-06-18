import os
import time
import serial
import binascii

ser = serial.Serial('/dev/ttyACM0', 9600, timout=1)