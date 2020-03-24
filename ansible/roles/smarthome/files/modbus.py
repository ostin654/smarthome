#!/usr/bin/env python

import serial
import json
import struct
from time import time
from time import sleep
import sys
from datetime import datetime
import signal
import os
from pymemcache.client import base
import re
import socket


def sendToZabbix(decoded_packet):
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.well.uptime -o %d" % (decoded_packet["well_uptime"])).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.well.water_level -o %f" % (7.0 * float(decoded_packet["water_level"]) / 1023)).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.uptime -o %d" % (decoded_packet["greenhouse_uptime"])).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.air_temp -o %f" % (float(decoded_packet["greenhouse_air_temp"]) / 1000)).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.soil_hum -o %d" % (decoded_packet["greenhouse_soil_hum"])).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.water_press -o %f" % (0.0146628 * float(decoded_packet["greenhouse_water_press"]) - 1.5)).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.valve_state -o %d" % (decoded_packet["greenhouse_relay_state"])).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.greenhouse.door_state -o %d" % (decoded_packet["greenhouse_door_state"])).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

socket.setdefaulttimeout(10)

port_serial = serial.Serial(
    port='/dev/ttyUSB2',
    baudrate = 9600,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1
)

for i in range(15):
    try:
        json_line = port_serial.readline()
        sendToZabbix(json.loads(json_line))
        break;
    except ValueError:
        print '%s -- Decoding JSON has failed, trying next... "%s"' % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', json_line))

