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


def writeNumber(address, value):
    bus.write_byte(address, value)
    return -1

def readNumber(address):
    number = bus.read_byte(address)
    return number

def readLong(address):
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

try:
    writeNumber(0x19, 0x99)
    writeNumber(0x19, 0x99)
    uptime = readLong(0x19) # mins
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.pressure.uptime -o %d" % (uptime)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x19, 0xa2)
    writeNumber(0x19, 0xa2)
    pressure = float(readLong(0x19)) * 40 / 1023 # kpa
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.pressure.current_pressure -o %f" % (pressure)).read()
    print res
except IOError:
    print "IOError"
    pass


try:
    writeNumber(0x20, 0xb9)
    writeNumber(0x20, 0xb9)
    well_uptime = readLong(0x20) # mins
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.well.uptime -o %d" % (well_uptime)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x20, 0xb0)
    writeNumber(0x20, 0xb0)
    water_level = float(readLong(0x20)) * 7 / 1023 # meter
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.well.water_level -o %f" % (water_level)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x20, 0xe9)
    writeNumber(0x20, 0xe9)
    greenhouse_uptime = readLong(0x20) # mins
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.uptime -o %d" % (greenhouse_uptime)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x20, 0xe0)
    writeNumber(0x20, 0xe0)
    soil_hum = readLong(0x20) # units
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.soil_hum -o %d" % (soil_hum)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x20, 0xe1)
    writeNumber(0x20, 0xe1)
    water_pressure = 0.0146628 * readLong(0x20) - 1.5 # bar
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.water_press -o %f" % (water_pressure)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x20, 0xe2)
    writeNumber(0x20, 0xe2)
    air_temp = readLong(0x20) * 0.001 # C
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.air_temp -o %f" % (air_temp)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x20, 0xe3)
    writeNumber(0x20, 0xe3)
    valve_state = readLong(0x20)
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.valve_state -o %d" % (valve_state)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x20, 0xe4)
    writeNumber(0x20, 0xe4)
    door_state = readLong(0x20)
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.door_state -o %d" % (door_state)).read()
    print res
except IOError:
    print "IOError"
    pass


try:
    writeNumber(0x18, 0xa0)
    writeNumber(0x18, 0xa0)
    lpg = readLong(0x18) # ppm
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.gas.current_lpg_level -o %d" % (lpg)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x18, 0xa1)
    writeNumber(0x18, 0xa1)
    methane = readLong(0x18) # ppm
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.gas.current_methane_level -o %d" % (methane)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x18, 0xc0)
    writeNumber(0x18, 0xc0)
    cur_temp = float(readLong(0x18)) / 16 # C
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.floorheat.current_temp -o %f" % (cur_temp)).read()
    print res
except IOError:
    print "IOError"
    pass

try:
    writeNumber(0x18, 0xc1)
    writeNumber(0x18, 0xc1)
    target_temp = readLong(0x18) # C
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.floorheat.target_temp -o %d" % (target_temp)).read()
    print res
except IOError:
    print "IOError"
    pass


