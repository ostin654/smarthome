from w1thermsensor import W1ThermSensor
from RPi import GPIO
import sqlite3
from time import time
from time import sleep
import sys
from datetime import datetime
import signal

PREFIX = "/var/lib/smarthome"

log = open(PREFIX+"/floorheat.log", "a")
log.write("%s Daemon started\n" % datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"))
log.close()

class GracefulKiller:
    kill_now = False
    def __init__(self):
        signal.signal(signal.SIGINT, self.exit_gracefully)
        signal.signal(signal.SIGTERM, self.exit_gracefully)

    def exit_gracefully(self,signum, frame):
        self.kill_now = True

file = open(PREFIX+"/floorheat_pin", "r")
pin = int(file.read())
file.close()

GPIO.setmode(GPIO.BCM)
GPIO.setup(pin, GPIO.OUT)

lastState = 'OFF'

historyConnection = sqlite3.connect(PREFIX+"/floorheat.db")
historyCursor = historyConnection.cursor()

while True:
    killer = GracefulKiller()
    try:
        file = open(PREFIX+"/floorheat_state", "r")
        state = file.read()
        file.close()

        log = open(PREFIX+"/floorheat.log", "a")

        if state == 'on':
            file = open(PREFIX+"/floorheat_target", "r")
            targetTemp = float(file.read())
            file.close()

            sensor = W1ThermSensor()
            actualTemp = sensor.get_temperature()

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
            log.write("%s Floorheat is swithed off\n" % datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"))

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

log = open(PREFIX+"/floorheat.log", "a")
log.write("%s Daemon terminated\n" % datetime.now().strftime("%A, %d. %B %Y %H:%M:%S"))
log.close()
