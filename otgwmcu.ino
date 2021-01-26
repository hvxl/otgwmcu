// NodeMCU firmware for the Nodo shop WiFi version of the OTGW
// Copyright (c) 2021 - Schelte Bron

#include "otgwmcu.h"
#include <Wire.h>
#include <WiFiManager.h>
#include "upgrade.h"
#include "proxy.h"
#include "debug.h"

#define WDTPERIOD 5000
#define WDADDRESS 38
#define WDPACKET 0xA5

char fwversion[16];

WiFiManager wifiManager;

// The Nodo shop OTGW hardware contains hardware that will reset
// the NodeMCU unless the watchdog timer is cleared periodically.
void clearwdt() {
  Wire.beginTransmission(WDADDRESS);
  Wire.write(WDPACKET);
  Wire.endTransmission();
}

void setup() {
  // Mount the file system
  LittleFS.begin();

  // Configure the serial port
  Serial.begin(9600, SERIAL_8N1);

  // Initialize the I/O ports
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, HIGH);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, HIGH);  

  // Setup I2C for the watchdog
  Wire.begin(I2CSDA, I2CSCL);

  // The PIC may have been confused by garbage on the
  // serial interface when the NodeMCU resets.
  picreset();

  // Setup WiFi
  wifiManager.setDebugOutput(false);
  // wifiManager.resetSettings();
  wifiManager.autoConnect("OTGW-MCU");

  // Prepare the Serial to Network Proxy
  proxysetup();

  debugsetup();
}

void loop() {
  static unsigned int wdevent = 0;
  if (millis() - wdevent > WDTPERIOD) {
    clearwdt();
    wdevent = millis();
  }

  // upgradeevent() will invoke proxyevent() if no
  // upgrade is in progress.
  upgradeevent();

  debugevent();
}
