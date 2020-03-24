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
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.pressure.uptime -o %d" % (decoded_packet["uptime"])).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.pressure.current_pressure -o %f" % (40.0 * float(decoded_packet["pressure"]) / 1023)).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

socket.setdefaulttimeout(10)

port_serial = serial.Serial(
    port='/dev/ttyUSB1',
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

