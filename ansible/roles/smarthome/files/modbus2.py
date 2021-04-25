from pymodbus.client.sync import ModbusSerialClient as ModbusClient
import time
import sys
from datetime import datetime
import signal
import os
import re
import socket

def log_message(message):
    print "%s -- %s" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), message)

def send_to_zabbix(key, value):
    cmd = "zabbix_sender -z s.smarthome.net.ru -p 10051 -s raspberrypi -k %s -o %f" % (key, value)
    log_message(cmd)
    res = os.popen(cmd).read()
    log_message(re.sub("\s+", ' ', res))

slave_config = (
#    {
#        'address':0x0a,
#        'start':1,
#        'registers':(
#            {'key': 'smarthome.well.uptime', 'multiplier':1.0, 'offset': 0.0},
#            {'key': 'smarthome.well.water_level', 'multiplier': 7.0/1023.0, 'offset': 0.0}
#        )
#    },
    {
        'address':0x0b,
        'start':1,
        'registers':(
            {'key': 'smarthome.greenhouse.uptime', 'multiplier':1.0, 'offset': 0.0},
            {'key': 'smarthome.greenhouse.air_temp', 'multiplier': 0.01, 'offset': 0.0},
#            {'key': 'smarthome.greenhouse.soil_hum', 'multiplier': 1.0, 'offset': 0.0},
#            {'key': 'smarthome.greenhouse.water_press', 'multiplier': 0.0146628, 'offset': -1.5},
            {'key': 'smarthome.greenhouse.valve_state', 'multiplier': 1.0, 'offset': 0.0},
            {'key': 'smarthome.greenhouse.door_state', 'multiplier': 1.0, 'offset': 0.0}
        )
    },
#    {
#        'address':0x0c,
#        'start':1,
#        'registers':(
#            {'key': 'smarthome.gastank.uptime', 'multiplier':1.0, 'offset': 0.0},
#            {'key': 'smarthome.gastank.gas_level', 'multiplier': 100.0/1023.0, 'offset': 0.0}
#        )
#    },
#    {
#        'address':0x0d,
#        'start':1,
#        'registers':(
#            {'key': 'smarthome.rx_433.uptime', 'multiplier':1.0, 'offset': 0.0},
#            {'key': 'smarthome.room.current_temp', 'multiplier': 0.01, 'offset': 0.0}
#        )
#    },
    {
        'address':0x0e,
        'start':1,
        'registers':(
            {'key': 'smarthome.pressure.uptime', 'multiplier':1.0, 'offset': 0.0},
            {'key': 'smarthome.pressure.current_pressure', 'multiplier': 10.0/1023.0, 'offset': 0.0}
        )
    },
    {
        'address':0x0f,
        'start':1,
        'registers':(
            {'key': 'smarthome.pressure.uptime', 'multiplier':1.0, 'offset': 0.0},
            {'key': 'smarthome.pressure.current_pressure', 'multiplier': 1.0, 'offset': 0.0}
        )
    }
)

socket.setdefaulttimeout(10)

client = ModbusClient(method='rtu', port=sys.argv[1], timeout=0.3, baudrate=38400)
client.connect()

for slave in slave_config:
    time.sleep(0.1)

    for i in range(5):
        response = client.read_input_registers(slave['start'], len(slave['registers']), unit=slave['address'])

        if response.isError():
            log_message("Error %#04x" % (slave['address']))
            time.sleep(3)
        else:
            for register in slave['registers']:
                send_to_zabbix(register['key'], response.registers[slave['registers'].index(register)] * register['multiplier'] + register['offset'])

            break
