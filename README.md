# Symlinks

* `/etc/systemd/system/homebridge.service -> ./systemd/homebridge.service`
* `/etc/default/homebridge -> ./default/homebridge`
* `/var/lib/homebridge/config.json -> ./homebridge/config.json`
* `/etc/monit/conf-enabled/thermostat -> ./monit/thermostat`
* `/var/lib/smarthome/thermostat.py -> ./thermostat/thermostat.py`
* `/etc/logrotate.d/thermostat -> ./logrotate/thermostat`

# Monitoring homebridge

`journalctl -f -u homebridge`

# Start/stop homebridge

`systemctl stop homebridge`

`systemctl start homebridge`

# Requirements

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
* `npm link`

# Dir structure

* `/var/lib/thermostat/actual`
* `/var/lib/thermostat/pin`
* `/var/lib/thermostat/state`
* `/var/lib/thermostat/target`
