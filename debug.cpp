// Copyright (c) 2021 - Schelte Bron

#include <ESP8266WiFi.h>
#include "otmon.h"

const int port = 45256;

WiFiServer debug(port);
WiFiClient debugClient;

void debuglog(const char *fmt, ...) {
  char buffer[256];
  int len;
  va_list argptr;

  if (debugClient) {
    va_start(argptr, fmt);
    len = vsprintf(buffer, fmt, argptr);
    va_end(argptr);
  
    if (debugClient.availableForWrite() >= len) {
      debugClient.write(buffer, len);
    }
  }
}

void debugmsg(char dir, unsigned message) {
  char buffer[256];
  int len;

  if (debugClient) {
    len = otformat(buffer, dir, message);
    len += sprintf(buffer + len, "\r\n");
    if (debugClient.availableForWrite() >= len) {
      debugClient.write(buffer, len);
    }
  }
}

void debugsetup() {
  debug.begin();
  debug.setNoDelay(true);
}

void debugevent() {
  //check if there are any new clients
  if (debug.hasClient()) {
    if (debugClient) {
      // Already connected
      debug.available().println("busy");
    } else {
      // Accept the connection
      debugClient = debug.available();
    }
  }
  
  while (debugClient.available()) {
    // Ignore any input
    debugClient.read();
  }
}
