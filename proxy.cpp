// Copyright (c) 2021 - Schelte Bron

#include <ESP8266WiFi.h>

#define MAX_SRV_CLIENTS 2

const int port = 25238;

WiFiServer server(port);
WiFiClient serverClients[MAX_SRV_CLIENTS];

void proxysetup() {
  server.begin();
  server.setNoDelay(true);
}

void proxyevent() {
  //check if there are any new clients
  if (server.hasClient()) {
    //find free/disconnected spot
    int i;
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (!serverClients[i]) { // equivalent to !serverClients[i].connected()
        serverClients[i] = server.available();
        break;
      }
    }
    
    //no free/disconnected spot so reject
    if (i == MAX_SRV_CLIENTS) {
      server.available().println("busy");
      // hints: server.available() is a WiFiClient with short-term scope
      // when out of scope, a WiFiClient will
      // - flush() - all data will be sent
      // - stop() - automatically too
    }
  }

  //check TCP clients for data
  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
    while (serverClients[i].available() && Serial.availableForWrite() > 0) {
      Serial.write(serverClients[i].read());
    }
  }

  //check UART for data
  size_t len = (size_t)Serial.available();
  if (len) {
    uint8_t sbuf[len];
    size_t serial_got = Serial.readBytes(sbuf, len);
    // push UART data to all connected telnet clients
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
      // if client.availableForWrite() was 0 (congested)
      // and increased since then,
      // ensure write space is sufficient:
      if (serverClients[i].availableForWrite() >= serial_got) {
        size_t tcp_sent = serverClients[i].write(sbuf, serial_got);
      }
  }
}
