// NodeMCU firmware for the Nodo shop WiFi version of the OTGW
// Copyright (c) 2021, 2022 - Schelte Bron

#include "otgwmcu.h"
#include <Wire.h>
#include <WiFiManager.h>
#include <TZ.h>
#include <Ticker.h>
#include <ESP8266httpUpdate.h>
#include "upgrade.h"
#include "proxy.h"
#include "debug.h"
#include "web.h"
#include "version.h"

#define WDTPERIOD 5000
#define WDADDRESS 38
#define WDPACKET 0xA5

const char updateurl[] = OTA_URL "/update.php";

OTGWSerial Pic(PICRST, LED2);

WiFiManager wifiManager;

// The Nodo shop OTGW hardware contains hardware that will reset
// the NodeMCU unless the watchdog timer is cleared periodically.
// https://github.com/letscontrolit/ESPEasy/blob/f05e6b8a6d/TinyI2CWatchdog/
void clearwdt() {
    Wire.beginTransmission(WDADDRESS);
    Wire.write(WDPACKET);
    Wire.endTransmission();
}

void enablewdt(bool enable = true) {
    // Enable/disable the ATtiny85 WDT by writing the action register
    Wire.beginTransmission(WDADDRESS);
    Wire.write(7);
    Wire.write(enable);
    Wire.endTransmission();
}

int dumpattiny(char *buffer) {
    int n = 0;
    for (int i = 0; i < 19; i++) {
        delay(1);
        Wire.beginTransmission(WDADDRESS);
        Wire.write(0x83);               // command to set pointer
        Wire.write(i);                  // pointer value to status byte
        Wire.endTransmission();
        delay(1);
        Wire.requestFrom(WDADDRESS, 1);
        if (Wire.available()) {
            n += sprintf_P(buffer + n, PSTR("%02x "), Wire.read());
        } else {
            return sprintf_P(buffer, PSTR("Not detected"));
        }
    }
    return --n;
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
    debuglog(PSTR("Upgrade finished: Errorcode = %d - %d retries, %d errors\n"), result, retries, errors);
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
    debuglog(PSTR("Upgrade: %d%%\n"), pct);
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

void wdtevent() {
    static unsigned int wdevent = 0;

    if (millis() - wdevent > WDTPERIOD) {
        clearwdt();
        wdevent = millis();
    }
}

void otaprogress(int n1, int n2) {
    debuglog(PSTR("OTA progress: %d%%\n"), 100 * n1 / n2);
    wdtevent();
}

void otaupgrade() {
    t_httpUpdate_return ret;
    WiFiClient wifi;
#if defined(NO_GLOBAL_INSTANCES) || defined(NO_GLOBAL_HTTPUPDATE)
    ESP8266HTTPUpdate ESPhttpUpdate;
#endif
    String version = VERSION;

    ESPhttpUpdate.setLedPin(LED1, LOW);
    ESPhttpUpdate.closeConnectionsOnUpdate(false);
    ESPhttpUpdate.rebootOnUpdate(false);
    ESPhttpUpdate.onProgress(otaprogress);
    ret = ESPhttpUpdate.update(wifi, updateurl, version);
    switch (ret) {
     case HTTP_UPDATE_FAILED:
        debuglog(PSTR("Firmware upgrade failed: %s\n"),
          ESPhttpUpdate.getLastErrorString().c_str());
        return;
     case HTTP_UPDATE_NO_UPDATES:
        debuglog(PSTR("No update available\n"));
        return;
     case HTTP_UPDATE_OK:
        debuglog(PSTR("Firmware updated\n"));
        break;
    }
    // Unmount the file system before updating
    LittleFS.end();
    // Perform an upgrade of the file system
    ret = ESPhttpUpdate.updateFS(wifi, updateurl, version);
    // Remount the file system, if possible
    LittleFS.begin();
    if (ret == HTTP_UPDATE_FAILED) {
        debuglog(PSTR("File system upgrade failed: %s\n"),
          ESPhttpUpdate.getLastErrorString().c_str());
    }
    debuglog(PSTR("OTA upgrade finished - Rebooting\n"));
    delay(100);
    ESP.restart();
}

void setup() {
    // Mount the file system
    LittleFS.begin();

    // Initialize the I/O ports
    pinMode(BUTTON, INPUT_PULLUP);
    pinMode(OTABTN, INPUT_PULLUP);
    pinMode(LED1, OUTPUT);
    digitalWrite(LED1, HIGH);
    pinMode(LED2, OUTPUT);
    digitalWrite(LED2, HIGH);  

    // Setup I2C for the watchdog
    Wire.begin(I2CSDA, I2CSCL);
    // The WDT should not interfere with the wifiManager connecting to WiFi.
    enablewdt(false);

    // Configure NTP before WiFi, so DHCP can override the NTP server(s)
    configTime(TZ_Europe_Amsterdam, "pool.ntp.org");

    // Setup WiFi
    wifiManager.setDebugOutput(false);
    // wifiManager.resetSettings();
    wifiManager.autoConnect("OTGW-MCU");

    // Activate the WDT
    clearwdt();
    enablewdt();

    // Prepare the Serial to Network Proxy
    proxysetup();
    debugsetup();
    websetup();

#ifdef DEBUG
    Pic.registerDebugFunc(debuglog);
#endif
}

void loop() {
    static unsigned int pressed = 0;

    wdtevent();

    if (!Pic.busy()) {
        if (digitalRead(BUTTON) == 0 || digitalRead(OTABTN) == 0) {
            if (pressed == 0) {
                // In the very unlikely case that millis() happens to be 0, the
                // user will have to press the button for one millisecond longer.
                pressed = millis();
            } else if (millis() - pressed > 2000) {
                if (digitalRead(OTABTN) == 0) {
                    otaupgrade();
                } else {
                    fwupgradestart(FIRMWARE);
                }
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
