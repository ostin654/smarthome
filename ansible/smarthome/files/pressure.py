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

address = 0x19
PREFIX = "/var/lib/smarthome"


while True:
    try:
        historyConnection = sqlite3.connect(PREFIX+"/smarthome2.db")
        historyCursor = historyConnection.cursor()

        writeNumber(0x99)
        uptime = readLong()                      # mins
        writeNumber(0xa2)
        pressure = float(readLong()) * 40 / 1023 # kpa

        log = open(PREFIX+"/i2cgw.log", "a")
        log.write("%s [pressure] Uptime %d Pressure %.2f kpa\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), uptime, pressure))

        historyCursor.execute("INSERT INTO pressure VALUES (NULL, ?, ?, ?)", [int(time()), uptime, pressure])
        historyConnection.commit()

        #print('Uptime ' + uptime + ' presure ' + pressure)
        #print("Uptime %d Pressure %.2f" % (uptime, pressure))

        historyConnection.close()
        log.close()
    except IOError:
        log = open(PREFIX+"/i2cgw.log", "a")
        log.write("%s [pressure] IOError\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S")))
        log.close()
        pass

    sleep(20)
