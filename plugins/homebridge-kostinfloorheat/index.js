{
  "name": "homebridge-kostinfloorheat",
  "version": "1.0.3",
  "description": "Kostin floor heater",
  "main": "index.js",
  "scripts": {},
  "keywords": [
    "homebridge-plugin",
    "kostinfloorheat"
  ],
  "repository": {
    "type": "git",
    "url": "https://example.com/git/kostinfloorheat.git"
  },
  "author": "Kostin Aleksey",
  "license": "None",
  "dependencies": {
    "memcached": "^2.2.2"
  },
  "engines": {
    "node": ">=0.12.0",
    "homebridge": ">=0.2.0"
  },
  "bugs": {
    "url": "https://example.com/"
  },
  "homepage": "https://example.com/"
}
root@raspberrypi:/home/pi/smarthome/plugins/homebridge-kostinfloorheat# cat index.js 

var Service, Characteristic, HomebridgeAPI, FakeGatoHistoryService, timeout;
 
module.exports = function (homebridge) {
  FakeGatoHistoryService = require('fakegato-history')(homebridge);
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  homebridge.registerAccessory("homebridge-kostinfloorheat", "KostinFloorHeat", KostinFloorHeat);
};

const moment = require('moment');
const memcached = require('memcached');

function KostinFloorHeat(log, config) {
  this.log = log;

  this.memcached = new memcached('127.0.0.1:11211');

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

    this.getLastData((err, data) => {
      if (err) {
        return this.log(err.message);
      }
      this.loggingService.addEntry({time: moment().unix(), currentTemp:data.currentTemp.toFixed(2), setTemp:data.setTemp.toFixed(2), valvePosition:data.valvePosition});
    });

    timeout = setTimeout(this.updateHistory.bind(this), 10 * 60 * 1000);
  },

  getLastData: function(next) {
    this.log('Getting last data');
    this.memcached.getMulti(['currentTemp', 'targetTemp', 'currentState', 'targetState', 'valvePosition'], next);
  },

  getCurrentState: function (next) {
    this.getLastData((err, data) => {
      if (err) {
        return next(err);
      }
      switch (date.currentState) {
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
    var setState;
    if (state == Characteristic.TargetHeatingCoolingState.AUTO) {
      setState = 'on';
    } else {
      setState = 'off';
    }
    this.memcached.set('setState', setState, 3600, (err) => {
      return next(err);
    });
  },

  getTargetState: function (next) {
    this.getLastData((err, data) => {
      if (err) {
        return next(err);
      }
      switch (data.targetState) {
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
    this.getLastData((err, data) => {
      if (err) {
        return next(err);
      }
      return next(null, data.currentTemp.toFixed(1));
    });
  },

  setTargetTemperature: function (target, next) {
    this.log('Setting target temperature');
    memcached.set('setTemp', target, 3600, (err) => {
      return next(err);
    });
  },

  getTargetTemperature: function (next) {
    this.getLastData((err, data) => {
      if (err) {
        return next(err);
      }
      return next(null, data.targetTemp.toFixed(1));
    });
  },

  setDisplayUnits: function (units, next) {
    return next();
  },

  getDisplayUnits: function (next) {
    return next(null, Characteristic.TemperatureDisplayUnits.CELSIUS);
  }

};