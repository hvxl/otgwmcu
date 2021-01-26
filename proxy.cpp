// Copyright (c) 2021 - Schelte Bron

#include <ESP8266WiFi.h>
#include "otgwmcu.h"
#include "debug.h"

#define ETX 0x04

#define MAX_SRV_CLIENTS 2

const int port = 25238;

WiFiServer proxy(port);
WiFiClient proxyClients[MAX_SRV_CLIENTS];

static char line[80];
static short linelen = 0;

void proxysetup() {
  proxy.begin();
  proxy.setNoDelay(true);
}

void proxyevent() {
  //check if there are any new clients
  if (proxy.hasClient()) {
    //find free/disconnected spot
    int i;
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (!proxyClients[i]) { // equivalent to !proxyClients[i].connected()
        proxyClients[i] = proxy.available();
        break;
      }
    }
    
    //no free/disconnected spot so reject
    if (i == MAX_SRV_CLIENTS) {
      proxy.available().println("busy");
      // hints: proxy.available() is a WiFiClient with short-term scope
      // when out of scope, a WiFiClient will
      // - flush() - all data will be sent
      // - stop() - automatically too
    }
  }

  //check TCP clients for data
  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
    while (proxyClients[i].available() && Serial.availableForWrite() > 0) {
      Serial.write(proxyClients[i].read());
    }
  }

  //check UART for data
  size_t len = (size_t)Serial.available();
  if (len) {
    size_t cnt = Serial.readBytesUntil('\n', line + linelen, min(len, sizeof(line) - linelen));
    linelen += cnt;
    // Check for ETX to allow a firmware upgrade by an external tool
    if (cnt < len || line[linelen - 1] == ETX) {
      if (line[linelen - 1] != ETX) {
        // Line of text is complete
        if (linelen < sizeof(line)) line[linelen++] = '\n';
      }

      // push the line to all connected telnet clients
      for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
        // if client.availableForWrite() was 0 (congested)
        // and increased since then,
        // ensure write space is sufficient:
        if (proxyClients[i].availableForWrite() >= linelen) {
          size_t tcp_sent = proxyClients[i].write(line, linelen);
        }
      }

      char *s = strstr(line, BANNER);
      if (s) {
        s += sizeof(BANNER);
        sscanf(s, "%s", fwversion);
        debuglog("Current firmware version: %s\n", fwversion);
      }

      linelen = 0;
    }
  }
}
