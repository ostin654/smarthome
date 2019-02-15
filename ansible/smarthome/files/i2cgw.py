# This Python file uses the following encoding: utf-8

import smbus
import struct
import sqlite3
from time import time
from time import sleep
import sys
from datetime import datetime
import signal


class GracefulKiller:
    kill_now = False
    def __init__(self):
        signal.signal(signal.SIGINT, self.exit_gracefully)
        signal.signal(signal.SIGTERM, self.exit_gracefully)

    def exit_gracefully(self,signum, frame):
        self.kill_now = True


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


bus = smbus.SMBus(1)
address = 0x18
PREFIX = "/var/lib/smarthome"


while True:
    killer = GracefulKiller()
    log = open(PREFIX+"/smarthome.log", "a")
    try:
        historyConnection = sqlite3.connect(PREFIX+"/smarthome.db")
        historyCursor = historyConnection.cursor()

        writeNumber(0xa0)
        lpg = readLong()                               # ppm
        writeNumber(0xa1)
        methane = readLong()                           # ppm
        writeNumber(0xa2)
        pressure = float(readLong()) * 15 / 1023 - 1.5 # bar
        writeNumber(0xb0)
        water_level = float(readLong()) * 7 / 1023     # meter

        writeNumber(0xc0)
        cur_temp = float(readLong()) / 16              # celsium
        writeNumber(0xc1)
        print readLong()
        target_temp = readLong()                       # celsium
        writeNumber(0xc2)
        print readLong()
        cur_state = 'HEAT' if readLong()>0 else 'COOL'
        writeNumber(0xc3)
        print readLong()
        target_state = 'AUTO' if readLong()>0 else 'OFF'

        file = open(PREFIX+"/floorheat_target_state", "r")
        targetState = float(file.read())
        file.close()
        file = open(PREFIX+"/floorheat_target_temp", "r")
        targetTemp = float(file.read())
        file.close()

        if targetState == 'on':
            writeNumber(0xd1)
        else:
            writeNumber(0xd0)

        writeNumber(int(targetTemp))


        log.write("%s Floor %.1f˚C => %.1f˚C Water %.2fm Gas %d/%d ppm Pressure %.2fb\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), cur_temp, target_temp, water_level, lpg, methane, pressure))

        historyCursor.execute("INSERT INTO floorheat VALUES (NULL, ?, ?, ?, ?, ?)", [int(time()), cur_temp, target_temp, cur_state, target_state])
        historyCursor.execute("INSERT INTO well VALUES (NULL, ?, ?)", [int(time()), water_level])
        historyCursor.execute("INSERT INTO gas VALUES (NULL, ?, ?, ?, ?)", [int(time()), lpg, methane, pressure])
        historyConnection.commit();

        historyConnection.close()
        log.close()

        if killer.kill_now:
            break
    except IOError:
        log.write("%s IOError\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S")))
        pass
    except KeyboardInterrupt:
        break

    sleep(60)
