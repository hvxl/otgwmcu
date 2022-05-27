// NodeMCU firmware for the Nodo shop WiFi version of the OTGW
// Copyright (c) 2021, 2022 - Schelte Bron

#include "otgwmcu.h"
#include <Wire.h>
#include <WiFiManager.h>
#include <TZ.h>
#include <Ticker.h>
#include "upgrade.h"
#include "proxy.h"
#include "debug.h"
#include "web.h"

#define WDTPERIOD 5000
#define WDADDRESS 38
#define WDPACKET 0xA5

OTGWSerial Pic(PICRST, LED2);

WiFiManager wifiManager;

// The Nodo shop OTGW hardware contains hardware that will reset
// the NodeMCU unless the watchdog timer is cleared periodically.
void clearwdt() {
    Wire.beginTransmission(WDADDRESS);
    Wire.write(WDPACKET);
    Wire.endTransmission();
}

void toggle() {
    int state = digitalRead(LED1);
    digitalWrite(LED1, !state);
}

void blink(short delayms = 200) {
    static Ticker blinker;
    if (delayms) {
        blinker.attach_ms(delayms, toggle);
    } else {
        blinker.detach();
        digitalWrite(LED1, HIGH);
    }
}

void fwupgradedone(OTGWError result, short errors = 0, short retries = 0) {
    debuglog("Upgrade finished: Errorcode = %d - %d retries, %d errors\n", result, retries, errors);
    switch (result) {
     case OTGW_ERROR_NONE:
        blink(0);
        break;
     case OTGW_ERROR_INPROG:
        break;
     case OTGW_ERROR_HEX_ACCESS:
     case OTGW_ERROR_HEX_FORMAT:
     case OTGW_ERROR_HEX_DATASIZE:
     case OTGW_ERROR_HEX_CHECKSUM:
     case OTGW_ERROR_MAGIC:
     case OTGW_ERROR_DEVICE:
        blink(500);
        break;
     default:
        blink(100);
        break;
    }
}

void fwupgradestep(int pct) {
    debuglog("Upgrade: %d%%\n", pct);
}

void fwupgradestart(const char *hexfile) {
    OTGWError result;

    blink(0);
    digitalWrite(LED1, LOW);
    result = Pic.startUpgrade(hexfile);
    if (result!= OTGW_ERROR_NONE) {
        fwupgradedone(result);
    } else {
        Pic.registerProgressCallback(fwupgradestep);
        Pic.registerFinishedCallback(fwupgradedone);
    }
}

void setup() {
    // Mount the file system
    LittleFS.begin();

    // Initialize the I/O ports
    pinMode(BUTTON, INPUT_PULLUP);
    pinMode(LED1, OUTPUT);
    digitalWrite(LED1, HIGH);
    pinMode(LED2, OUTPUT);
    digitalWrite(LED2, HIGH);  

    // Setup I2C for the watchdog
    Wire.begin(I2CSDA, I2CSCL);

    // Configure NTP before WiFi, so DHCP can override the NTP server(s)
    configTime(TZ_Europe_Amsterdam, "pool.ntp.org");

    // Setup WiFi
    wifiManager.setDebugOutput(false);
    // wifiManager.resetSettings();
    wifiManager.autoConnect("OTGW-MCU");

    // Prepare the Serial to Network Proxy
    proxysetup();
    debugsetup();
    websetup();

#ifdef DEBUG
    Pic.registerDebugFunc(debuglog);
#endif
}

void loop() {
    static unsigned int wdevent = 0, pressed = 0;

    if (millis() - wdevent > WDTPERIOD) {
        clearwdt();
        wdevent = millis();
    }

    if (!Pic.busy()) {
        if (digitalRead(BUTTON) == 0) {
            if (pressed == 0) {
                // In the very unlikely case that millis() happens to be 0, the 
                // user will have to press the button for one millisecond longer.
                pressed = millis();
            } else if (millis() - pressed > 2000) {
                fwupgradestart(FIRMWARE);
                pressed = 0;
            }
        } else {
            pressed = 0;
        }
    }

    proxyevent();
    debugevent();
    webevent();
}
