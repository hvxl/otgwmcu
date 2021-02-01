// Copyright (c) 2021 - Schelte Bron

#include <Arduino.h>

const char msgtype0[] PROGMEM = "Read-Data";
const char msgtype1[] PROGMEM = "Write-Data";
const char msgtype2[] PROGMEM = "Inv-Data";
const char msgtype3[] PROGMEM = "Reserved";
const char msgtype4[] PROGMEM = "Read-Ack";
const char msgtype5[] PROGMEM = "Write-Ack";
const char msgtype6[] PROGMEM = "Data-Inv";
const char msgtype7[] PROGMEM = "Unk-DataId";

const char* const msgtypes[] PROGMEM = {
  msgtype0, msgtype1, msgtype2, msgtype3,
  msgtype4, msgtype5, msgtype6, msgtype7
}; 

// Class 1: Control and Status Information
const char otmsgid000[] PROGMEM = "Status";
const char otmsgid001[] PROGMEM = "Control setpoint";
const char otmsgid005[] PROGMEM = "Application-specific flags";
const char otmsgid008[] PROGMEM = "Control setpoint 2";
const char otmsgid070[] PROGMEM = "Status V/H";
const char otmsgid071[] PROGMEM = "Control setpoint V/H";
const char otmsgid072[] PROGMEM = "Fault flags/code V/H";
const char otmsgid073[] PROGMEM = "OEM diagnostic code V/H";
const char otmsgid101[] PROGMEM = "Solar storage mode and status";
const char otmsgid102[] PROGMEM = "Solar storage fault flags";
const char otmsgid115[] PROGMEM = "OEM diagnostic code";

// Class 2: Configuration Information
const char otmsgid002[] PROGMEM = "Master configuration";
const char otmsgid003[] PROGMEM = "Slave configuration";
const char otmsgid074[] PROGMEM = "Configuration/memberid V/H";
const char otmsgid075[] PROGMEM = "OpenTherm version V/H";
const char otmsgid076[] PROGMEM = "Product version V/H";
const char otmsgid103[] PROGMEM = "Solar storage config/memberid";
const char otmsgid104[] PROGMEM = "Solar storage product version";
const char otmsgid124[] PROGMEM = "OpenTherm version Master";
const char otmsgid125[] PROGMEM = "OpenTherm version Slave";
const char otmsgid126[] PROGMEM = "Master product version";
const char otmsgid127[] PROGMEM = "Slave product version";

// Class 3: Remote Commands
const char otmsgid004[] PROGMEM = "Remote command";

// Class 4: Sensor and Informational Data
const char otmsgid016[] PROGMEM = "Room setpoint";
const char otmsgid017[] PROGMEM = "Relative modulation level";
const char otmsgid018[] PROGMEM = "CH water pressure";
const char otmsgid019[] PROGMEM = "DHW flow rate";
const char otmsgid020[] PROGMEM = "Day of week and time of day";
const char otmsgid021[] PROGMEM = "Date";
const char otmsgid022[] PROGMEM = "Year";
const char otmsgid023[] PROGMEM = "Room Setpoint CH2";
const char otmsgid024[] PROGMEM = "Room temperature";
const char otmsgid025[] PROGMEM = "Boiler water temperature";
const char otmsgid026[] PROGMEM = "DHW temperature";
const char otmsgid027[] PROGMEM = "Outside temperature";
const char otmsgid028[] PROGMEM = "Return water temperature";
const char otmsgid029[] PROGMEM = "Solar storage temperature";
const char otmsgid030[] PROGMEM = "Solar collector temperature";
const char otmsgid031[] PROGMEM = "Flow temperature CH2";
const char otmsgid032[] PROGMEM = "DHW2 temperature";
const char otmsgid033[] PROGMEM = "Exhaust temperature";
const char otmsgid034[] PROGMEM = "Boiler heat exchanger temperature";
const char otmsgid035[] PROGMEM = "Boiler fan speed and setpoint";
const char otmsgid036[] PROGMEM = "Flame current";
const char otmsgid037[] PROGMEM = "Room temperature CH2";
const char otmsgid038[] PROGMEM = "Relative humidity";
const char otmsgid077[] PROGMEM = "Relative ventilation";
const char otmsgid078[] PROGMEM = "Relative humidity exhaust air";
const char otmsgid079[] PROGMEM = "CO2 level exhaust air";
const char otmsgid080[] PROGMEM = "Supply inlet temperature";
const char otmsgid081[] PROGMEM = "Supply outlet temperature";
const char otmsgid082[] PROGMEM = "Exhaust inlet temperature";
const char otmsgid083[] PROGMEM = "Exhaust outlet temperature";
const char otmsgid084[] PROGMEM = "Exhaust fan speed";
const char otmsgid085[] PROGMEM = "Inlet fan speed";
const char otmsgid098[] PROGMEM = "RF sensor status information";
const char otmsgid109[] PROGMEM = "Electricity producer starts";
const char otmsgid110[] PROGMEM = "Electricity producer hours";
const char otmsgid111[] PROGMEM = "Electricity production";
const char otmsgid112[] PROGMEM = "Cumulative electricity production";
const char otmsgid113[] PROGMEM = "Unsuccessful burner starts";
const char otmsgid114[] PROGMEM = "Flame signal too low count";
const char otmsgid116[] PROGMEM = "Burner starts";
const char otmsgid117[] PROGMEM = "CH pump starts";
const char otmsgid118[] PROGMEM = "DHW pump/valve starts";
const char otmsgid119[] PROGMEM = "DHW burner starts";
const char otmsgid120[] PROGMEM = "Burner operation hours";
const char otmsgid121[] PROGMEM = "CH pump operation hours";
const char otmsgid122[] PROGMEM = "DHW pump/valve operation hours";
const char otmsgid123[] PROGMEM = "DHW burner operation hours";

// Class 5: Pre-defined Remote Boiler Parameters
const char otmsgid006[] PROGMEM = "Remote parameter flags";
const char otmsgid048[] PROGMEM = "DHW setpoint boundaries";
const char otmsgid049[] PROGMEM = "Max CH setpoint boundaries";
const char otmsgid050[] PROGMEM = "OTC heat curve ratio boundaries";
const char otmsgid051[] PROGMEM = "Remote parameter 4 boundaries";
const char otmsgid052[] PROGMEM = "Remote parameter 5 boundaries";
const char otmsgid053[] PROGMEM = "Remote parameter 6 boundaries";
const char otmsgid054[] PROGMEM = "Remote parameter 7 boundaries";
const char otmsgid055[] PROGMEM = "Remote parameter 8 boundaries";
const char otmsgid056[] PROGMEM = "DHW setpoint";
const char otmsgid057[] PROGMEM = "Max CH water setpoint";
const char otmsgid058[] PROGMEM = "OTC heat curve ratio";
const char otmsgid059[] PROGMEM = "Remote parameter 4";
const char otmsgid060[] PROGMEM = "Remote parameter 5";
const char otmsgid061[] PROGMEM = "Remote parameter 6";
const char otmsgid062[] PROGMEM = "Remote parameter 7";
const char otmsgid063[] PROGMEM = "Remote parameter 8";
const char otmsgid086[] PROGMEM = "Remote parameter settings V/H";
const char otmsgid087[] PROGMEM = "Nominal ventilation value";

// Class 6: Transparent Slave Parameters
const char otmsgid010[] PROGMEM = "Number of TSPs";
const char otmsgid011[] PROGMEM = "TSP setting";
const char otmsgid088[] PROGMEM = "Number of TSPs V/H";
const char otmsgid089[] PROGMEM = "TSP setting V/H";
const char otmsgid105[] PROGMEM = "Number of TSPs solar storage";
const char otmsgid106[] PROGMEM = "TSP setting solar storage";

// Class 7: Fault History Data
const char otmsgid012[] PROGMEM = "Size of fault buffer";
const char otmsgid013[] PROGMEM = "Fault buffer entry";
const char otmsgid090[] PROGMEM = "Size of fault buffer V/H";
const char otmsgid091[] PROGMEM = "Fault buffer entry V/H";
const char otmsgid107[] PROGMEM = "Size of fault buffer solar storage";
const char otmsgid108[] PROGMEM = "Fault buffer entry solar storage";

// Class 8: Control of Special Applications
const char otmsgid007[] PROGMEM = "Cooling control signal";
const char otmsgid009[] PROGMEM = "Remote override room setpoint";
const char otmsgid014[] PROGMEM = "Maximum relative modulation level";
const char otmsgid015[] PROGMEM = "Boiler capacity and modulation limits";
const char otmsgid099[] PROGMEM = "Remote override operating mode heating";
const char otmsgid100[] PROGMEM = "Remote override function";

const char* const msgids[] PROGMEM = {
  otmsgid000, otmsgid001, otmsgid002, otmsgid003, otmsgid004,
  otmsgid005, otmsgid006, otmsgid007, otmsgid008, otmsgid009,
  otmsgid010, otmsgid011, otmsgid012, otmsgid013, otmsgid014,
  otmsgid015, otmsgid016, otmsgid017, otmsgid018, otmsgid019,
  otmsgid020, otmsgid021, otmsgid022, otmsgid023, otmsgid024,
  otmsgid025, otmsgid026, otmsgid027, otmsgid028, otmsgid029,
  otmsgid030, otmsgid031, otmsgid032, otmsgid033, otmsgid034,
  otmsgid035, otmsgid036, otmsgid037, otmsgid038, nullptr,
  nullptr,    nullptr,    nullptr,    nullptr,    nullptr,
  nullptr,    nullptr,    nullptr,    otmsgid048, otmsgid049,
  otmsgid050, otmsgid051, otmsgid052, otmsgid053, otmsgid054,
  otmsgid055, otmsgid056, otmsgid057, otmsgid058, otmsgid059,
  otmsgid060, otmsgid061, otmsgid062, otmsgid063, nullptr,
  nullptr,    nullptr,    nullptr,    nullptr,    nullptr,
  otmsgid070, otmsgid071, otmsgid072, otmsgid073, otmsgid074,
  otmsgid075, otmsgid076, otmsgid077, otmsgid078, otmsgid079,
  otmsgid080, otmsgid081, otmsgid082, otmsgid083, otmsgid084,
  otmsgid085, otmsgid086, otmsgid087, otmsgid088, otmsgid089,
  otmsgid090, otmsgid091, nullptr,    nullptr,    nullptr,
  nullptr,    nullptr,    nullptr,    otmsgid098, otmsgid099,
  otmsgid100, otmsgid101, otmsgid102, otmsgid103, otmsgid104,
  otmsgid105, otmsgid106, otmsgid107, otmsgid108, otmsgid109,
  otmsgid110, otmsgid111, otmsgid112, otmsgid113, otmsgid114,
  otmsgid115, otmsgid116, otmsgid117, otmsgid118, otmsgid119,
  otmsgid120, otmsgid121, otmsgid122, otmsgid123, otmsgid124,
  otmsgid125, otmsgid126, otmsgid127
};

enum {
  OTFORMATNONE,
  OTFORMATDATE,
  OTFORMATTIME,
  OTFORMATRFSENSOR,
  OTFORMATOVERRIDE,
  OTFORMATFLOAT,
  OTFORMATFLAGFLAG,
  OTFORMATFLAGUBYTE,
  OTFORMATFLAGLB,
  OTFORMATUBYTELB,
  OTFORMATINTEGER,
  OTFORMATBYTEBYTE,
  OTFORMATUNSIGNED,
  OTFORMATUBYTEHB,
  OTFORMATUBYTEUBYTE
};

const char msgfmts[] PROGMEM = {
  OTFORMATFLAGFLAG,   OTFORMATFLOAT,      OTFORMATFLAGUBYTE,  OTFORMATFLAGUBYTE,
  OTFORMATUBYTEUBYTE, OTFORMATFLAGUBYTE,  OTFORMATFLAGFLAG,   OTFORMATFLOAT,
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATUBYTEHB,    OTFORMATUBYTEUBYTE,
  OTFORMATUBYTEHB,    OTFORMATUBYTEUBYTE, OTFORMATFLOAT,      OTFORMATUBYTEUBYTE,
  // 16
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,
  OTFORMATTIME,       OTFORMATDATE,       OTFORMATUNSIGNED,   OTFORMATFLOAT,
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,
  // 32
  OTFORMATFLOAT,      OTFORMATINTEGER,    OTFORMATFLOAT,      OTFORMATUBYTEUBYTE,
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATNONE,
  OTFORMATNONE,       OTFORMATNONE,       OTFORMATNONE,       OTFORMATNONE,
  OTFORMATNONE,       OTFORMATNONE,       OTFORMATNONE,       OTFORMATNONE,
  // 48
  OTFORMATBYTEBYTE,   OTFORMATBYTEBYTE,   OTFORMATBYTEBYTE,   OTFORMATBYTEBYTE,
  OTFORMATBYTEBYTE,   OTFORMATBYTEBYTE,   OTFORMATBYTEBYTE,   OTFORMATBYTEBYTE,
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,
  // 64
  OTFORMATNONE,       OTFORMATNONE,       OTFORMATNONE,       OTFORMATNONE,
  OTFORMATNONE,       OTFORMATNONE,       OTFORMATFLAGFLAG,   OTFORMATUBYTELB,
  OTFORMATFLAGUBYTE,  OTFORMATUNSIGNED,   OTFORMATFLAGUBYTE,  OTFORMATFLOAT,
  OTFORMATUBYTEUBYTE, OTFORMATUBYTELB,    OTFORMATUBYTEUBYTE, OTFORMATUNSIGNED,
  // 80
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATFLOAT,
  OTFORMATUNSIGNED,   OTFORMATUNSIGNED,   OTFORMATFLAGFLAG,   OTFORMATUBYTEHB,
  OTFORMATUBYTEHB,    OTFORMATUBYTEUBYTE, OTFORMATUBYTEHB,    OTFORMATUBYTEUBYTE,
  OTFORMATNONE,       OTFORMATNONE,       OTFORMATNONE,       OTFORMATNONE,
  // 96
  OTFORMATNONE,       OTFORMATNONE,       OTFORMATRFSENSOR,   OTFORMATOVERRIDE,
  OTFORMATFLAGLB,     OTFORMATFLAGFLAG,   OTFORMATFLAGUBYTE,  OTFORMATFLAGUBYTE,
  OTFORMATUBYTEUBYTE, OTFORMATUBYTEHB,    OTFORMATUBYTEUBYTE, OTFORMATUBYTEUBYTE,
  OTFORMATUBYTEUBYTE, OTFORMATUNSIGNED,   OTFORMATUNSIGNED,   OTFORMATUNSIGNED,
  // 112
  OTFORMATUNSIGNED,   OTFORMATUNSIGNED,   OTFORMATUNSIGNED,   OTFORMATUNSIGNED,
  OTFORMATUNSIGNED,   OTFORMATUNSIGNED,   OTFORMATUNSIGNED,   OTFORMATUNSIGNED,
  OTFORMATUNSIGNED,   OTFORMATUNSIGNED,   OTFORMATUNSIGNED,   OTFORMATUNSIGNED,
  OTFORMATFLOAT,      OTFORMATFLOAT,      OTFORMATUBYTEUBYTE, OTFORMATUBYTEUBYTE
};

const char datetimestr[] PROGMEM = {
  "Unk\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat\0Sun\0"
  "Unk\0Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0"
};
