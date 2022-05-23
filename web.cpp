// Copyright (c) 2021 - Schelte Bron

#include "otgwmcu.h"
#include "webserver.h"
#include "debug.h"
#include "otmon.h"
#include <LittleFS.h>
#include <ESP8266HTTPClient.h>

WebServer httpd(80);

// Bitmaps for subscriptions of web socket clients
static unsigned int ws_status, ws_otlog;
static unsigned int updays = 0;

const char *hexheaders[] = {
    "Last-Modified",
    "X-Version"
};

unsigned int uptime() {
    static unsigned int tstamp = 0;
    unsigned int up, ms = millis();

    up = ms - tstamp;
    if (up >= 86400000) {
        updays++;
        tstamp += 86400000;
    }
    return up;
}

int running(char *buffer) {
    unsigned int h, m, s, ms;
    ms = uptime() % 86400000;
    h = ms / 3600000;
    ms = ms % 3600000;
    m = ms / 60000;
    ms = ms % 60000;
    s = ms / 1000;
    ms = ms % 1000;
    return sprintf_P(buffer, PSTR("%lu days %02d:%02d:%02d.%03d"), updays, h, m, s, ms);
}

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

void debuginfo() {
    FSInfo fsinfo;
    char buffer[250];
    int cnt;

    httpd.chunkedResponseModeStart(200, "text/html");
    httpd.sendContent_P(PSTR("<!DOCTYPE html>\n<html>\n<head>\n"
      "<link rel=\"icon\" type=\"image/png\" href=\"favicon.png\">\n"
      "<meta charset=\"utf-8\"/>\n<title>Slimme meter</title>\n"
      "<style>\nbody {\n  background: white;\n}\n</style>\n"
      "</head>\n<body>\n"));
    // MAC address
    sprintf_P(buffer, PSTR("MAC Address: %s<br>\n"), WiFi.macAddress().c_str());
    httpd.sendContent(buffer);
    // IP address
    sprintf_P(buffer, PSTR("IP Address: %s<br>\n"), WiFi.localIP().toString().c_str());
    httpd.sendContent(buffer);
    // System uptime
    cnt = sprintf_P(buffer, PSTR("Uptime: "));
    cnt += running(buffer + cnt);
    cnt += sprintf_P(buffer + cnt, PSTR("<br>\n"));
    httpd.sendContent(buffer, cnt);
    // Last reset reason
    String str = ESP.getResetReason();
    sprintf_P(buffer, PSTR("Reset reason: %s<br>\n"), str.c_str());
    httpd.sendContent(buffer);
    // File system usage
    FSInfo fs;
    LittleFS.info(fs);
    sprintf_P(buffer, PSTR("File system: %d%% (%d/%d)<br>\n"), 100 * fs.usedBytes / fs.totalBytes, fs.usedBytes, fs.totalBytes);
    httpd.sendContent(buffer);
    // Sketch usage
    const int sketchtotal = 1044464;
    int sketchused = ESP.getSketchSize();
    sprintf_P(buffer, PSTR("Program: %d%% (%d/%d)<br>\n"), 100 * sketchused / sketchtotal, sketchused, sketchtotal);
    httpd.sendContent(buffer);
    // Memory usage
    const int memtotal = 81920;
    int memused = memtotal - ESP.getFreeHeap();
    sprintf_P(buffer, PSTR("Memory: %d%% (%d/%d)<br>\n"), 100 * memused / memtotal, memused, memtotal);
    httpd.sendContent(buffer);
    // Memory fragmentation
    sprintf_P(buffer, PSTR("Fragmentation: %d%%<br>\n"), ESP.getHeapFragmentation());
    httpd.sendContent(buffer);
    // Platform versions
    sprintf_P(buffer, PSTR("Core version: %s<br>SDK version: %s<br>\n"),
      ESP.getCoreVersion().c_str(), ESP.getSdkVersion());
    httpd.sendContent(buffer);

    httpd.sendContent_P(PSTR("</body>\n</html>\n"));
    httpd.chunkedResponseFinalize();
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
     case WStype_CONNECTED:     // if a new websocket connection is established
        debuglog("[%u] Connected!\n", num);
        if (clientmap) *clientmap |= 1 << num;
        return true;
     case WStype_DISCONNECTED:  // if the websocket is disconnected
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
    httpd.on("/debug.html", HTTP_GET, debuginfo);
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
    uptime();
}
