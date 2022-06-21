// Copyright (c) 2021, 2022 - Schelte Bron

#include <Arduino.h>
#include <LittleFS.h>
#include <OTGWSerial.h>

#define I2CSCL D1
#define I2CSDA D2
#define BUTTON D3
#define PICRST D5
#define OTABTN D6

#define LED1 D4
#define LED2 D0

#define FIRMWARE "gateway.hex"
#define OTA_URL "http://otgw.tclcode.com/ota"

extern OTGWSerial Pic;

void wdtevent();
int dumpattiny(char *buffer);
void fwupgradestart(const char *hexfile);
String otaurl();
void otaupgrade();
