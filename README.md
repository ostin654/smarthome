# Symlinks

* `/etc/systemd/system/homebridge.service -> ./systemd/homebridge.service`
* `/etc/default/homebridge -> ./default/homebridge`
* `/var/lib/homebridge/config.json -> ./homebridge/config.json`
* `/etc/monit/conf-enabled/floorheat -> ./monit/floorheat`
* `/etc/monit/conf-enabled/well -> ./monit/well`
* `/var/lib/smarthome/floorheat.py -> ./floorheat/floorheat.py`
* `/etc/logrotate.d/floorheat -> ./logrotate/floorheat`

# Monitoring homebridge

`journalctl -f -u homebridge`

# Start/stop homebridge

`systemctl stop homebridge`

`systemctl start homebridge`

# Requirements

* https://github.com/xpertsavenue/WiringOP-Zero (for Orange Pi)

```
git clone https://github.com/xpertsavenue/WiringOP-Zero.git
cd WiringOP-Zero
chmod +x ./build
sudo ./build
```

* https://github.com/nRF24/RF24

```
git clone https://github.com/nRF24/RF24.git
cd RF24
./configure
make
sudo make install
```

# Python requirements

* W1ThermSensor
* GPIO
* sqlite3
* time
* datetime

# Sqlite schema

`sqlite3 /var/lib/smarthome/floorheat.db`

```
CREATE TABLE history(Id INTEGER PRIMARY KEY AUTOINCREMENT, Time INTEGER, CurrentTemperature REAL, TargetTemperature REAL, CurrentState TEXT, TargetState TEXT);
CREATE INDEX time_index ON history(Time);
```

`sqlite3 /var/lib/smarthome/well.db`

```
CREATE TABLE history(Id INTEGER PRIMARY KEY AUTOINCREMENT, Time INTEGER, WaterLevel REAL);
CREATE INDEX time_index ON history(Time);
```

# Install NPM modules

* `npm install -g sqlite3 --unsafe-perm`
* `npm install -g moment --unsafe-perm`
* `npm install -g fs --unsafe-perm`
* `npm install -g fakegato-history --unsafe-perm`

# Link NPM modules

* `npm link sqlite3`
* `npm link moment`
* `npm link fs`
* `npm link fakegato-history`
* `sudo npm link --unsafe-perm`

# Dir structure

* `/var/lib/smarthome/heatfloor_actual`
* `/var/lib/smarthome/heatfloor_pin`
* `/var/lib/smarthome/heatfloor_state`
* `/var/lib/smarthome/heatfloor_target`
