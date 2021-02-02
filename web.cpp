// Copyright (c) 2021 - Schelte Bron

#include "webserver.h"
#include "upgrade.h"
#include "debug.h"
#include "otmon.h"
#include <LittleFS.h>
#include <ESP8266HTTPClient.h>

WebServer httpd(80);

// Bitmaps for subscriptions of web socket clients
static unsigned int ws_status, ws_otlog;

const char *hexheaders[] = {
  "Last-Modified",
  "X-Version"
};

bool servefile(String path) {
  if (path.endsWith("/")) {
    path += "index.html";
  }

  String contentType;
  contentType = mime::getContentType(path);

  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    httpd.streamFile(file, contentType);
    file.close();
    return true;
  } else {
    return false;
  }
}

void listfiles() {
  Dir dir = LittleFS.openDir("/");
  httpd.chunkedResponseModeStart(200, "text/plain");

  while (dir.next()) {
    httpd.sendContent(dir.fileName() + "\n");
  }
  httpd.chunkedResponseFinalize();
}

void servefilesys() {
  String uri = ESP8266WebServer::urlDecode(httpd.uri());
  if (servefile(uri)) return;
  if (uri.endsWith("/")) {
    listfiles();
  }
  httpd.send(404, "text/plain", "Not found");
}

void mainpage() {
  servefile("/index.html");
}

void filelist() {
  char *s, buffer[400], version[16];
  Dir dir;
  File f;
  
  s += sprintf(s = buffer, "var ls=[");
  dir = LittleFS.openDir("/");
  while (dir.next()) {
    if (dir.fileName().endsWith(".hex")) {
      String verfile = "/" + dir.fileName();
      verfile.replace(".hex", ".ver");
      f = LittleFS.open(verfile, "r");
      if (f) {
        f.readBytesUntil('\n', version, 15);
        f.close();
      } else {
        sprintf(version, "0.0");
      }
      s += sprintf(s, "{name:'%s',version:'%s',size:%d},", dir.fileName().c_str(), version, dir.fileSize());
    }
  }
  s += sprintf(s, "]\n");
  httpd.send(200, "text/javascript", buffer);
}

void refresh(String filename, String version) {
  WiFiClient client;
  HTTPClient http;
  String latest;
  int code;
  
  http.begin(client, "http://otgw.tclcode.com/download/" + filename);
  http.collectHeaders(hexheaders, 2);
  code = http.sendRequest("HEAD");
  if (code == HTTP_CODE_OK) {
    for (int i = 0; i< http.headers(); i++) {
      debuglog("%s: %s\n", hexheaders[i], http.header(i).c_str());
    }
    latest = http.header(1);
    if (latest != version) {
      debuglog("Update %s: %s -> %s\n", filename.c_str(), version.c_str(), latest.c_str());
      http.end();
      http.begin(client, "http://otgw.tclcode.com/download/" + filename);
      code = http.GET();
      if (code == HTTP_CODE_OK) {
        File f = LittleFS.open("/" + filename, "w");
        if (f) {
          http.writeToStream(&f);
          f.close();
          String verfile = "/" + filename;
          verfile.replace(".hex", ".ver");
          f = LittleFS.open(verfile, "w");
          if (f) {
            f.print(latest + "\n");
            f.close();
            debuglog("Update successful\n");
          }
        }
      }
    }
  }
  http.end();
}

void firmware() {
  String action = httpd.arg("action");
  String filename = httpd.arg("name");
  String version = httpd.arg("version");
  debuglog("Action: %s %s\n", action.c_str(), filename.c_str());
  if (action == "download") {
    fwupgradestart(String("/" + filename).c_str());
  } else if (action == "update") {
    refresh(filename, version);
  } else if (action == "delete") {
    String path = "/" + filename;
    LittleFS.remove(path);
    path.replace(".hex", ".ver");
    LittleFS.remove(path);
  }
  httpd.sendHeader("Location", "firmware.html", true);
  httpd.send(303, "text/html", "<a href='firmware.html'>Return</a>");
}

// Web sockets
unsigned int websockdistribute(const char *str, unsigned int clients) {
  unsigned int n, fail = 0;
  uint8_t num;

  for (n = clients, num = 0; n != 0; n >>= 1, num++) {
    if (n & 1)
      if (!httpd.sendTXT(num, str))
        fail++;
  }
  return fail;
}

bool websocket(uint8_t num, WStype_t type, unsigned int *clientmap) {
  switch (type) {
   case WStype_CONNECTED:                 // if a new websocket connection is established
    debuglog("[%u] Connected!\n", num);
    if (clientmap) *clientmap |= 1 << num;
    return true;
   case WStype_DISCONNECTED:              // if the websocket is disconnected
    debuglog("[%u] Disconnected!\n", num);
    if (clientmap) *clientmap &= ~(1 << num);
    break;
  default:
    break;
  }
  return false;
}

void websocketsend(int num, char *str) {
  httpd.sendTXT(num, str);
}

void websockreport(char *json) {
  if (ws_status != 0) {
    websockdistribute(json, ws_status);
  }
}

void wsstatus(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (websocket(num, type, &ws_status)) {
    initialreport(num);
  }
}

void websockstatus() {
  httpd.upgrade(wsstatus);
}

void websockotmessage(char dir, unsigned message) {
  char buffer[256];
  int len;

  if (ws_otlog != 0) {
    len = otformat(buffer, dir, message);
    websockdistribute(buffer, ws_otlog);
  }
}

void wsotlog(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  websocket(num, type, &ws_otlog);
}

void websockotlog() {
  httpd.upgrade(wsotlog);
}

void upload() {
  static File fsUploadFile;
  String filename;

  HTTPUpload& upload = httpd.upload();
  switch (upload.status) {
   case UPLOAD_FILE_START:
    filename = upload.filename;
    filename = httpd.arg("target") + "/" + filename;
    debuglog("handleFileUpload Name: %s\n", filename.c_str());
    // Open the file for writing (create if it doesn't exist)
    fsUploadFile = LittleFS.open(filename, "w");
    break;
   case UPLOAD_FILE_WRITE:
    // Write the received bytes to the file
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
    break;
   case UPLOAD_FILE_END:
    if (fsUploadFile) {
      fsUploadFile.close();
      debuglog("handleFileUpload Size: %d\n", upload.totalSize);
      // Redirect the client to the success page
      httpd.sendHeader("Location","upload.html");
      httpd.send(303);
    } else {
      httpd.send(500, "text/plain", "500: couldn't create file");
    }
    break;
  }
}

void websetup() {
  // Serve files from flash file system
  httpd.onNotFound(servefilesys);
  // Special web pages
  httpd.on("/", HTTP_GET, mainpage);
  httpd.on("/filelist.js", HTTP_GET, filelist);
  httpd.on("/firmware.html", HTTP_POST, firmware);
  // Web sockets
  httpd.on("/status.ws", HTTP_GET, websockstatus);
  httpd.on("/otlog.ws", HTTP_GET, websockotlog);
  // Can't pass a nullptr as the main handler function. Instead use
  // an anonymous function without args and an empty body: [](){}
  httpd.on("/upload.html", HTTP_POST, [](){}, upload);
  
  httpd.begin();
}

void webevent() {
  httpd.handleClient();
}
