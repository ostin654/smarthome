
var Service, Characteristic, HomebridgeAPI, timeout;
 
module.exports = function (homebridge) {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  homebridge.registerAccessory("homebridge-kostinwell", "KostinWell", KostinWell);
};

const fs = require('fs');
const sqlite3 = require('sqlite3').verbose();
const moment = require('moment');

function KostinWell(log, config) {
  this.log = log;
  this.database = typeof config["database"]  !== 'undefined' ? config["database"] : '/var/lib/smarthome/well.db';
  this.table = typeof config["table"]  !== 'undefined' ? config["table"] : 'history';
}

KostinWell.prototype = {
  getServices: function () {
    this.log('Getting Services');
    let informationService = new Service.AccessoryInformation();
    informationService
      .setCharacteristic(Characteristic.Manufacturer, "Kostin Aleksey")
      .setCharacteristic(Characteristic.Model, "Well1")
      .setCharacteristic(Characteristic.SerialNumber, "200-1000001");

    let wellService = new Service.HumidifierDehumidifier("Well");

    wellService
      .getCharacteristic(Characteristic.CurrentRelativeHumidity)
        .on('get', this.getCurrentRelativeHumidity.bind(this));

    wellService
      .getCharacteristic(Characteristic.CurrentHumidifierDehumidifierState)
        .on('get', this.getCurrentHumidifierDehumidifierState.bind(this));

    wellService
      .getCharacteristic(Characteristic.TargetHumidifierDehumidifierState)
        .on('set', this.setTargetHumidifierDehumidifierState.bind(this))
        .on('get', this.getTargetHumidifierDehumidifierState.bind(this));

    wellService
      .getCharacteristic(Characteristic.Active)
        .on('set', this.setActive.bind(this))
        .on('get', this.getActive.bind(this));

    wellService
      .getCharacteristic(Characteristic.WaterLevel)
        .on('get', this.getWaterLevel.bind(this));

    this.informationService = informationService;
    this.wellService = wellService;
    return [informationService, wellService];
  },

  getCurrentRelativeHumidity: function(next) {
    this.log('Getting CurrentRelativeHumidity');
    return next(null, 0);
  },
  
  getCurrentHumidifierDehumidifierState: function(next) {
    this.log('Getting CurrentHumidifierDehumidifierState');
    return next(null, Characteristic.CurrentHumidifierDehumidifierState.INACTIVE);
  },
  
  setTargetHumidifierDehumidifierState: function (next) {
    this.log('Setting TargetHumidifierDehumidifierState');
    return next();
  },
  
  getTargetHumidifierDehumidifierState: function (next) {
    this.log('Getting TargetHumidifierDehumidifierState');
    return next(null, Characteristic.TargetHumidifierDehumidifierState.HUMIDIFIER);
  },
  
  setActive: function (next) {
    this.log('Setting active');
    return next();
  },
  
  getActive: function (next) {
    this.log('Getting active');
    return next(null, Characteristic.Active.INACTIVE);
  },

  getWaterLevel: function (next) {
    this.log('Getting waterlevel');
    this.getLastDataSQLite((err, row) => {
      if (err) {
        return next(err);
      }
      return next(null, row.WaterLevel);
    });
  },
  
  getLastDataSQLite: function(next) {
    this.log('Getting data from SQLite');
    let db = new sqlite3.Database(this.database, sqlite3.OPEN_READONLY, (err) => {
      if (err) {
        return next(err);
      }
    });
    db.get('select * from '+this.table+' order by Time desc limit 1', [], next);
    db.close();
  }
};
