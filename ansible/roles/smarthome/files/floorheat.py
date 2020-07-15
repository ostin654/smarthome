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
import urllib2
import socket


def sendToZabbix(decoded_packet):
    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.gas.current_lpg_level -o %d" % (decoded_packet["lpg"])).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.gas.current_methane_level -o %d" % (decoded_packet["methane"])).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.floorheat.current_temp -o %f" % (float(decoded_packet["current_temp"])/16)).read()
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', res))

    res = os.popen("zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k smarthome.floorheat.target_temp -o %d" % (decoded_packet["target_temp"])).read()
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


hdr = {'User-Agent': 'Smarthome floorheat', 'Accept': '*/*'}

request = urllib2.Request('https://f.smarthome.net.ru/target_temp', headers=hdr)
try:
    response = urllib2.urlopen(request)
    target_temp = int(response.read())
    is_temp = True
except urllib2.HTTPError, e:
    print '%s -- %s' % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), e.fp.read())
    is_temp = False

request = urllib2.Request('https://f.smarthome.net.ru/target_state', headers=hdr)
try:
    response = urllib2.urlopen(request)
    target_state = int(response.read())
    is_state = True
except urllib2.HTTPError, e:
    print '%s -- %s' % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), e.fp.read())
    is_state = False


if is_temp and is_state:
    for i in range(3):
        port_serial.write("{\"target_temp\":%d, \"target_state\": %d}\n" % (int(target_temp), int(target_state)))
        print '%s -- Setting temperature %d, %d ...' % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), int(target_temp), int(target_state))
        sleep(2)



for i in range(15):
    try:
        json_line = port_serial.readline()
        sendToZabbix(json.loads(json_line))
        break;
    except ValueError:
        print '%s -- Decoding JSON has failed, trying next... "%s"' % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), re.sub("\s+", ' ', json_line))
