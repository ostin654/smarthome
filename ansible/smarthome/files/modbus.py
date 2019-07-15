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

address = 0x20
PREFIX = "/var/lib/smarthome"


while True:
    try:
        historyConnection = sqlite3.connect(PREFIX+"/smarthome2.db")
        historyCursor = historyConnection.cursor()

        writeNumber(0xb9)
        well_uptime = readLong()                      # mins
        writeNumber(0xb0)
        water_level = float(readLong()) * 7 / 1023    # meter

        writeNumber(0xe9)
        greenhouse_uptime = readLong()                # mins
        writeNumber(0xe0)
        soil_hum = readLong()                         # units
        writeNumber(0xe1)
        water_pressure = 0.0146628 * readLong() - 1.5 # bar
        writeNumber(0xe2)
        air_temp = readLong() * 0.001                 # C
        writeNumber(0xe3)
        valve_state = readLong()
        writeNumber(0xe4)
        door_state = readLong()

        log = open(PREFIX+"/i2cgw.log", "a")
        log.write("%s [well] Uptime %d Water %.2fm\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), well_uptime, water_level))
        log.write("%s [greenhouse] Uptime %d Soil %d Pressure %.2fb Air %.2f˚C Valve %d Door %d\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), greenhouse_uptime, soil_hum, water_pressure, air_temp, valve_state, door_state))

        historyCursor.execute("INSERT INTO well VALUES (NULL, ?, ?, ?)", [int(time()), well_uptime, water_level])
        historyCursor.execute("INSERT INTO greenhouse VALUES (NULL, ?, ?, ?, ?, ?, ?, ?)", [int(time()), greenhouse_uptime, soil_hum, water_pressure, air_temp, valve_state, door_state])
        historyConnection.commit()
        historyConnection.close()
        log.close()
    except IOError:
        log = open(PREFIX+"/i2cgw.log", "a")
        log.write("%s [well] IOError\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S")))
        log.close()
        pass

    sleep(20)