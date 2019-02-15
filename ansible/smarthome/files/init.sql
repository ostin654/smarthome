CREATE TABLE IF NOT EXISTS floorheat(Id INTEGER PRIMARY KEY AUTOINCREMENT, Time INTEGER, CurrentTemperature REAL, TargetTemperature REAL, CurrentState TEXT, TargetState TEXT);
CREATE INDEX IF NOT EXISTS floorheat_time_index ON floorheat(Time);
CREATE TABLE IF NOT EXISTS well(Id INTEGER PRIMARY KEY AUTOINCREMENT, Time INTEGER, WaterLevel REAL);
CREATE INDEX IF NOT EXISTS well_time_index ON well(Time);
CREATE TABLE IF NOT EXISTS gas(Id INTEGER PRIMARY KEY AUTOINCREMENT, Time INTEGER, CurrentLpgLevel REAL, CurrentMethaneLevel REAL, CurrentGasPressure REAL);
CREATE INDEX IF NOT EXISTS gas_time_index ON floorheat(Time);
