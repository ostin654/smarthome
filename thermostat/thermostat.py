from w1thermsensor import W1ThermSensor
from RPi import GPIO
import sqlite3
from time import time
from time import sleep
import sys
from datetime import datetime
import signal

log = open("/var/lib/thermostat/thermostat.log", "a")
log.write("%s Daemon started\n" % datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"))
log.close()

class GracefulKiller:
    kill_now = False
    def __init__(self):
        signal.signal(signal.SIGINT, self.exit_gracefully)
        signal.signal(signal.SIGTERM, self.exit_gracefully)

    def exit_gracefully(self,signum, frame):
        self.kill_now = True

file = open("/var/lib/thermostat/pin", "r")
pin = int(file.read())
file.close()

GPIO.setmode(GPIO.BCM)
GPIO.setup(pin, GPIO.OUT)

lastState = 'OFF'

historyConnection = sqlite3.connect('/var/lib/thermostat/floorheat.db')
historyCursor = historyConnection.cursor()

while True:
    killer = GracefulKiller()
    try:
        file = open("/var/lib/thermostat/state", "r")
        state = file.read()
        file.close()

        log = open("/var/lib/thermostat/thermostat.log", "a")

        if state == 'on':
            file = open("/var/lib/thermostat/target", "r")
            targetTemp = float(file.read())
            file.close()

            sensor = W1ThermSensor()
            actualTemp = sensor.get_temperature()

            file = open("/var/lib/thermostat/actual", "w")
            file.write("%s" % actualTemp)
            file.close()

            log.write("%s Actual temperature is %.1f˚C\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), actualTemp))

            if actualTemp > targetTemp + 5 :
                GPIO.output(pin, False)
                log.write("%s Need to cool => %.1f˚C\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), targetTemp))
                lastState = 'COOL'
            if actualTemp < targetTemp - 0 :
                GPIO.output(pin, True)
                log.write("%s Need to head => %.1f˚C\n" % (datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"), targetTemp))
                lastState = 'HEAT'

            historyCursor.execute("INSERT INTO history VALUES (NULL, ?, ?, ?, ?, ?)", [int(time()), actualTemp, targetTemp, lastState, 'AUTO'])
            historyConnection.commit();

        else:
            GPIO.output(pin, False)
            log.write("%s Thermostat is swithed off\n" % datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"))

            lastState = 'OFF'

            historyCursor.execute("INSERT INTO history VALUES (NULL, ?, NULL, NULL, ?, ?)", [int(time()), 'OFF', 'OFF'])
            historyConnection.commit();

        log.close()
        sleep(20)

        if killer.kill_now:
            break
    except KeyboardInterrupt:
        GPIO.cleanup()
        historyConnection.close()

        print("Bye")
        sys.exit()

GPIO.cleanup()
historyConnection.close()

log = open("/var/lib/thermostat/thermostat.log", "a")
log.write("%s Daemon terminated\n" % datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"))
log.close()
