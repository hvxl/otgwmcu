// Copyright (c) 2021 - Schelte Bron

#include "webserver.h"
#include "debug.h"
#include <Hash.h>
#include <base64.h>

#define WEBSOCKETS_MAX_HEADER_SIZE 14

const char* websockheaders[] {
  "Connection",
  "Sec-WebSocket-Key",
  "Sec-WebSocket-Version",
  "Upgrade",
  "Referer"
};

// WebSocket class
// Constructor
WebSocket::WebSocket(uint8_t id, WiFiClient &client)
: _client(client), _id(id), _dataLen(0), _callback(nullptr) {
  // Do not block while waiting for data
  _client.setTimeout(0);

  debuglog("Connection opened from: %s:%d\n", _client.remoteIP().toString().c_str(), _client.remotePort());
}

// Destructor
WebSocket::~WebSocket()
{
  _client.stop();
  debuglog("Connection closed\n");
}

void WebSocket::disconnect(uint16_t code) {
  uint8_t buffer[2];
  buffer[0] = code >> 8 & 0xff;
  buffer[1] = code & 0xff;
  sendFrame(WSop_close, buffer, 2);
  if (_callback) {
    _callback(_id, WStype_DISCONNECTED, nullptr, 0);
  }
}

void WebSocket::callback(wsCallback cb)
{
  _callback = cb;
  if (_callback) {
    _callback(_id, WStype_CONNECTED, nullptr, 0);
  }
}

bool WebSocket::loop() {
  uint8_t len;
  int hdrSize;
  bool ret = true;

  if (!_client.connected()) {
    return false;
  }
  if (_dataLen > _dataSize) {
    if (_client.available()) {
      _dataSize += _client.read(_payload + _dataSize, _dataLen - _dataSize);
      if (_dataSize == _dataLen) {
        uint8_t *mask = _header + 2;
        len = _header[1] & 0x7f;
        if (len == 126) mask += 2;
        if (len == 127) mask += 8;
        for (size_t i = 0; i < _dataLen; i++) {
          _payload[i] ^= mask[i & 3];
        }
        WSopcode_t opcode = (WSopcode_t)(_header[0] & 0x7f);
        switch (opcode) {
          case WSop_text:
            if (_callback) {
              _payload[_dataLen] = '\0';
              _callback(_id, WStype_TEXT, _payload, _dataLen);
            }
            break;
          case WSop_binary:
            if (_callback) {
              _callback(_id, WStype_BIN, _payload, _dataLen);
            }
            break;
          case WSop_continuation:
            break;
          case WSop_ping:
            sendFrame(WSop_pong, _payload, _dataLen);
            break;
          case WSop_pong:
            break;
          case WSop_close: {
            uint16_t reason = 1000;
            if (_dataLen >= 2) {
              reason = _payload[0] << 8 | _payload[1];
            }
            disconnect(reason);
            ret = false;
            break;
          }
          default:
            disconnect(1002);
            ret = false;
            break;
        }
        _dataLen = 0;
      }
    }
  } else if (_client.available() >= 2) {
    _client.peekBytes(_header, 2);
    len = _header[1] & 0x7f;
    if (len == 126) {
      hdrSize = 4;
    } else if (len == 127) {
      hdrSize = 10;
    } else {
      hdrSize = 2;
    }
    bool mask = _header[1] & bit(7);
    if (mask) {
      hdrSize += 4;
    }
    unsigned int hdrPtr = 2;
    if (_client.available() >= hdrSize) {
      _client.read(_header, hdrSize);
      if (len < 126) {
        _dataLen = len;
      } else if (len == 126) {
        _dataLen = _header[hdrPtr++];
        _dataLen = _dataLen << 8 | _header[hdrPtr++];
      } else {
        hdrPtr += 4;
        _dataLen = _dataLen << 8 | _header[hdrPtr++];
        _dataLen = _dataLen << 8 | _header[hdrPtr++];
      }
      _dataSize = 0;
    }
  }
  return ret;
}

bool WebSocket::sendFrame(WSopcode_t opcode, uint8_t * payload, size_t length)
{
  uint8_t buffer[WEBSOCKETS_MAX_HEADER_SIZE] = {0};
  uint8_t headerSize;
  uint8_t *headerPtr;
  bool ret = true, mask = false, fin = true;

  if (length < 126) {
    headerSize = 2;
  } else if (length < 0xFFFF) {
    headerSize = 4;
  } else {
    headerSize = 10;
  }

  headerPtr = buffer;

  // Create header

  // Opcode & Fin
  *headerPtr = opcode;
  if (fin) {
    *headerPtr |= bit(7);
  }
  headerPtr++;

  // Length & Mask
  *headerPtr = mask ? bit(7) : 0;
  if (length < 126) {
    *headerPtr++ |= length;
  } else if (length < 0xFFFF) {
    *headerPtr++ |= 126;
    *headerPtr++ = ((length >> 8) & 0xFF);
    *headerPtr++ = (length & 0xFF);
  } else {
    *headerPtr++ |= 127;
    *headerPtr++ = 0;
    *headerPtr++ = 0;
    *headerPtr++ = 0;
    *headerPtr++ = 0;
    *headerPtr++ = ((length >> 24) & 0xFF);
    *headerPtr++ = ((length >> 16) & 0xFF);
    *headerPtr++ = ((length >> 8) & 0xFF);
    *headerPtr++ = (length & 0xFF);
  }

  // Send header and payload
  if (_client.write(buffer, headerSize) != headerSize) {
    ret = false;
  } else if (_client.write(payload, length) != length) {
    ret = false;
  }

  return ret;
}

bool WebSocket::sendTXT(const char *str) {
  if (_client.connected()) {
    return sendFrame(WSop_text, (uint8_t *)str, strlen(str));
  } else {
    return false;
  }
}

// WebServer class
WebServer::WebServer(int port)
: ESP8266WebServer(port) {
  // Specify the important headers
  collectHeaders(websockheaders, 5);
}

void WebServer::handleClient() {
  int i;

  ESP8266WebServer::handleClient();
  for (i = 0; i < WEBSOCKETS_CLIENT_MAX; i++) {
    if (_wsclients[i]) {
      if (!_wsclients[i]->loop()) {
        delete _wsclients[i];
        _wsclients[i] = nullptr;
      }
    }
  }
}

int WebServer::upgrade(wsCallback cb) {
  uint8_t ws;
  String acceptKey;

  // Find a free websocket slot
  for (ws = 0; ws < WEBSOCKETS_CLIENT_MAX; ws++) {
    if (_wsclients[ws] == NULL) break;
  }
  if (ws >= WEBSOCKETS_CLIENT_MAX) {
    send(503, "text/plain", "Max Websockets exceeded");
    return -1;
  }
  acceptKey = wschecks();
  if (acceptKey.length() == 0) {
    send(400, "text/plain", "Websocket failed");
    return -1;
  }

  _wsclients[ws] = new WebSocket(ws, _currentClient);

  // Accept the websocket connection
  String handshake = "HTTP/1.1 101 Switching Protocols\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "Sec-WebSocket-Accept: ";
  handshake += acceptKey + "\r\n\r\n";
  _currentClient.print(handshake);

  // Don't let the standard processing close the connection
  _currentClient = WiFiClient();

  // Configure the callback function
  _wsclients[ws]->callback(cb);

  return ws;
}

bool WebServer::sendTXT(int num, const char *str)
{
  return (_wsclients[num] && _wsclients[num]->sendTXT(str));
}

String WebServer::wschecks() {
  String ret, headerValue;

  // Check the websocket request according to RFC 6455.
  // * The method of the request MUST be GET
  if (_currentMethod != HTTP_GET) {
    return ret;
  }
  // * The request MUST contain an |Upgrade| header field whose value MUST include the "websocket" keyword.
  headerValue = header("Upgrade");
  headerValue.toLowerCase();
  if (headerValue.indexOf("websocket") < 0) {
      return ret;
  }
  // * The request MUST contain a |Connection| header field whose value MUST include the "Upgrade" token.
  headerValue = header("Connection");
  headerValue.toLowerCase();
  if (headerValue.indexOf("upgrade") < 0) {
    return ret;
  }
  // * The request MUST include a header field with the name |Sec-WebSocket-Version|.
  // * The value of this header field MUST be 13.
  headerValue = header("Sec-WebSocket-Version");
  if (headerValue.toInt() != 13) {
    // Websocket draft implementation is also acceptable
    if (headerValue.toInt() != 8) {
      return ret;
    }
  }
  // * The request MUST include a header field with the name |Sec-WebSocket-Key|.
  headerValue = header("Sec-WebSocket-Key");
  if (headerValue.length() != 24) {
    return ret;
  }

  // Websocket request passed all tests
  // Generate the accept key value
  uint8_t sha1HashBin[20] = {0};
  sha1(headerValue + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", sha1HashBin);
  ret = base64::encode(sha1HashBin, 20, false);
  return ret;
}
