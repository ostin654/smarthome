# This Python file uses the following encoding: utf-8

import smbus
import struct
import sqlite3
from time import time
from time import sleep
import sys
from datetime import datetime
import signal
import os


def writeNumber(value):
    bus.write_byte(address, value)
    return -1

def readNumber():
    number = bus.read_byte(address)
    return number

def readLong():
    number = bus.read_byte(address)
    number = (number << 8) | bus.read_byte(address)
    number = (number << 8) | bus.read_byte(address)
    number = (number << 8) | bus.read_byte(address)
    return swap32(number)

def swap32(i):
    return struct.unpack("<I", struct.pack(">I", i))[0]


if os.path.exists("/dev/i2c-0"):
    bus = smbus.SMBus(0)
elif os.path.exists("/dev/i2c-1"):
    bus = smbus.SMBus(1)
else:
    raise Exception("No bus /dev/i2c*")

address = 0x18
PREFIX = "/var/lib/smarthome"


while True:
    try:
        historyConnection = sqlite3.connect(PREFIX+"/smarthome.db")
        historyCursor = historyConnection.cursor()

        writeNumber(0xa0)
        lpg = readLong()                               # ppm
        writeNumber(0xa1)
        methane = readLong()                           # ppm
        #writeNumber(0xa2)
        #pressure = float(readLong()) * 51.2 / 1023     # kpa
        #writeNumber(0xb0)
        #water_level = float(readLong()) * 7 / 1023     # meter

        writeNumber(0xc0)
        cur_temp = float(readLong()) / 16              # celsium
        writeNumber(0xc1)
        target_temp = readLong()                       # celsium
        writeNumber(0xc2)
        cur_state = 'HEAT' if readLong()>0 else 'COOL'
        writeNumber(0xc3)
        target_state = 'AUTO' if readLong()>0 else 'OFF'

        file = open(PREFIX+"/floorheat_target_state", "r")
        targetState = file.read()
        file.close()
        file = open(PREFIX+"/floorheat_target_temp", "r")
        targetTemp = float(file.read())
        file.close()

        if targetState == 'on':
            writeNumber(0xd1)
        else:
            writeNumber(0xd0)

        writeNumber(int(targetTemp))


        log = open(PREFIX+"/i2cgw.log", "a")
        log.write("%s Floor %.1f˚C => %.1f˚C (%s) Gas %d/%d ppm\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), cur_temp, target_temp, cur_state, lpg, methane))

        historyCursor.execute("INSERT INTO floorheat VALUES (NULL, ?, ?, ?, ?, ?)", [int(time()), cur_temp, target_temp, cur_state, target_state])
        #historyCursor.execute("INSERT INTO well VALUES (NULL, ?, ?)", [int(time()), water_level])
        historyCursor.execute("INSERT INTO gas VALUES (NULL, ?, ?, ?, 0)", [int(time()), lpg, methane])
        historyConnection.commit();

        historyConnection.close()
        log.close()
    except IOError:
        log = open(PREFIX+"/i2cgw.log", "a")
        log.write("%s IOError\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S")))
        log.close()
        pass

    sleep(20)