
var Service, Characteristic, HomebridgeAPI, FakeGatoHistoryService, timeout;
 
module.exports = function (homebridge) {
  FakeGatoHistoryService = require('fakegato-history')(homebridge);
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  homebridge.registerAccessory("homebridge-kostinfloorheat", "KostinFloorHeat", KostinFloorHeat);
};

const sqlite3 = require('sqlite3').verbose();
const moment = require('moment');

function KostinFloorHeat(log, config) {
  this.log = log;
  this.database = typeof config["database"]  !== 'undefined' ? config["database"] : '/var/lib/smarthome/floorheat.db';
  this.table = typeof config["table"]  !== 'undefined' ? config["table"] : 'history';
  this.settings_table = typeof config["settings_table"]  !== 'undefined' ? config["settings_table"] : 'settings';
  this.loggingService = new FakeGatoHistoryService("thermo", this);
  timeout = setTimeout(this.updateHistory.bind(this), 10 * 60 * 1000);
}

KostinFloorHeat.prototype = {
  getServices: function () {
    let informationService = new Service.AccessoryInformation();
    informationService
      .setCharacteristic(Characteristic.Manufacturer, "Kostin Aleksey")
      .setCharacteristic(Characteristic.Model, "FloorHeater3")
      .setCharacteristic(Characteristic.SerialNumber, "100-1000003");

    let thermostatService = new Service.Thermostat("Floor heat");
    thermostatService
      .getCharacteristic(Characteristic.CurrentHeatingCoolingState)
        .on('get', this.getCurrentState.bind(this));

    thermostatService
      .getCharacteristic(Characteristic.TargetHeatingCoolingState)
        .on('set', this.setTargetState.bind(this))
        .on('get', this.getTargetState.bind(this));

    thermostatService
      .getCharacteristic(Characteristic.CurrentTemperature)
        .on('get', this.getCurrentTemperature.bind(this));

    thermostatService
      .getCharacteristic(Characteristic.TargetTemperature)
        .on('set', this.setTargetTemperature.bind(this))
        .on('get', this.getTargetTemperature.bind(this));

    thermostatService
      .getCharacteristic(Characteristic.TemperatureDisplayUnits)
        .on('set', this.setDisplayUnits.bind(this))
        .on('get', this.getDisplayUnits.bind(this));

    this.informationService = informationService;
    this.thermostatService = thermostatService;
    return [informationService, thermostatService, this.loggingService];
  },

  updateHistory: function() {
    this.log('Updating history');
    let db = new sqlite3.Database(this.database, (err) => {
      if (err) {
        return this.log(err.message);
      }
    });
    db.get("select avg(CurrentTemperature) as currentTemp, avg(TargetTemperature) as setTemp, case when CurrentState='HEAT' then 100 else 0 end as valvePosition from "+this.table+" where Time > (julianday('now') - 2440587.5) * 86400.0 - 600", [], (err, row) => {
      if (err) {
        return this.log(err.message);
      }
      this.loggingService.addEntry({time: moment().unix(), currentTemp:row.currentTemp.toFixed(2), setTemp:row.setTemp.toFixed(2), valvePosition:row.valvePosition});
    });
    db.close();

    timeout = setTimeout(this.updateHistory.bind(this), 10 * 60 * 1000);
  },

  getLastData: function(next) {
    this.log('Getting last data');
    let db = new sqlite3.Database(this.database, (err) => {
      if (err) {
        return next(err);
      }
    });
    db.get('select * from '+this.table+' order by Time desc limit 1', [], next);
    db.close();
  },

  getCurrentState: function (next) {
    this.getLastData((err, row) => {
      if (err) {
        return next(err);
      }
      switch (row.CurrentState) {
        case 'OFF':
          return next(null, Characteristic.CurrentHeatingCoolingState.OFF);
          break;
        case 'COOL':
          return next(null, Characteristic.CurrentHeatingCoolingState.COOL);
          break;
        case 'HEAT':
          return next(null, Characteristic.CurrentHeatingCoolingState.HEAT);
          break;
      }
    });
  },

  setTargetState: function (state, next) {
    this.log('Setting target state');
    let db = new sqlite3.Database(this.database, (err) => {
      if (err) {
        return next(err);
      }
    });
    if (state == Characteristic.TargetHeatingCoolingState.AUTO) {
      db.run('UPDATE '+this.settings_table+' SET TargetState=?', ['AUTO'], next);
    } else {
      db.run('UPDATE '+this.settings_table+' SET TargetState=?', ['OFF'], next);
    }
    db.close();
  },

  getTargetState: function (next) {
    this.getLastData((err, row) => {
      if (err) {
        return next(err);
      }
      switch (row.TargetState) {
        case 'OFF':
          return next(null, Characteristic.TargetHeatingCoolingState.OFF);
          break;
        case 'COOL':
          return next(null, Characteristic.TargetHeatingCoolingState.COOL);
          break;
        case 'HEAT':
          return next(null, Characteristic.TargetHeatingCoolingState.HEAT);
          break;
        case 'AUTO':
          return next(null, Characteristic.TargetHeatingCoolingState.AUTO);
          break;
      }
    });
  },

  getCurrentTemperature: function (next) {
    this.getLastData((err, row) => {
      if (err) {
        return next(err);
      }
      return next(null, row.CurrentTemperature.toFixed(1));
    });
  },

  setTargetTemperature: function (target, next) {
    this.log('Setting target temperature');
    let db = new sqlite3.Database(this.database, (err) => {
      if (err) {
        return next(err);
      }
    });
    db.run('UPDATE '+this.settings_table+' SET TargetTemperature=?', [target.toString()], next);
    db.close();
  },

  getTargetTemperature: function (next) {
    this.getLastData((err, row) => {
      if (err) {
        return next(err);
      }
      return next(null, row.TargetTemperature.toFixed(1));
    });
  },

  setDisplayUnits: function (units, next) {
    return next();
  },

  getDisplayUnits: function (next) {
    return next(null, Characteristic.TemperatureDisplayUnits.CELSIUS);
  }

};
