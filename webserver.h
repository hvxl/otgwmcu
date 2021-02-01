// Copyright (c) 2021 - Schelte Bron
#include <ESP8266WebServer.h>

#define WEBSOCKETS_CLIENT_MAX 8
#define MAX_PAYLOAD_SIZE 500

typedef enum {
  WStype_ERROR,
  WStype_DISCONNECTED,
  WStype_CONNECTED,
  WStype_TEXT,
  WStype_BIN,
  WStype_FRAGMENT_TEXT_START,
  WStype_FRAGMENT_BIN_START,
  WStype_FRAGMENT,
  WStype_FRAGMENT_FIN,
} WStype_t;

typedef enum {
  WSop_continuation = 0x00, ///< %x0 denotes a continuation frame
  WSop_text = 0x01,         ///< %x1 denotes a text frame
  WSop_binary = 0x02,       ///< %x2 denotes a binary frame
                            ///< %x3-7 are reserved for further non-control frames
  WSop_close = 0x08,        ///< %x8 denotes a connection close
  WSop_ping = 0x09,         ///< %x9 denotes a ping
  WSop_pong = 0x0A          ///< %xA denotes a pong
                            ///< %xB-F are reserved for further control frames
} WSopcode_t;

typedef void (*wsCallback)(uint8_t, WStype_t, uint8_t *, size_t);

class WebSocket {
public:
  WebSocket(uint8_t, WiFiClient&);
  virtual ~WebSocket();

  virtual void callback(wsCallback);
  virtual bool loop();
  virtual void disconnect(uint16_t code);
  bool sendTXT(const char *);

protected:
  WiFiClient _client;
  uint8_t _id;
  uint8_t _header[14];
  uint8_t _payload[MAX_PAYLOAD_SIZE];
  size_t _dataLen, _dataSize;
  wsCallback _callback;
  bool sendFrame(WSopcode_t, uint8_t *, size_t);
};

class WebServer : public ESP8266WebServer {
public:
  WebServer(int port = 80);

  virtual void handleClient();
  virtual int upgrade(wsCallback);
  bool sendTXT(int, const char *);

protected:
  WebSocket *_wsclients[WEBSOCKETS_CLIENT_MAX];
  String wschecks();
};
