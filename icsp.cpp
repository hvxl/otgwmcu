#include "icsp.h"
#include "debug.h"
#include <LittleFS.h>

#define ICSPDAT D7
#define ICSPCLK D6
#define PICMCLR D5

// https://forum.arduino.cc/t/arduino-esp8266-faster-direct-port-write/407251

#define KEY "MCHP"

#define LOADCONFIG	0x00
#define LOADPROGMEM	0x02
#define LOADDATAMEM	0x03
#define READPROGMEM	0x04
#define READDATAMEM	0x05
#define INCRADDRESS	0x06
#define RESETADDRESS	0x16
#define PROGINTERNAL	0x08
#define PROGEXTERNAL	0x18
#define ENDEXTERNAL	0x0A
#define ERASEPROGMEM	0x09
#define ERASEDATAMEM	0x0B
#define ERASEPROGROW	0x11

typedef enum {
    ICSPIDLE,
    ICSPPROGPROG,
    ICSPPROGCONF,
    ICSPPROGDATA,
    ICSPVFYPROG,
    ICSPVFYCONF,
    ICSPVFYDATA
} icspstate_t;

typedef enum {
    ICSPMODEIDLE,
    ICSPMODEPROG,
    ICSPMODEVFY
} icspmode_t;

static uint16_t pc = 0, hexseg = 0, row = 0, used = 0, errcnt = 0, linenum;
static icspstate_t state = ICSPIDLE;
static icspmode_t mode = ICSPMODEIDLE;
static File fd;

void icsp_setup() {
    // Initialize the I/O ports
    pinMode(ICSPCLK, INPUT);
    pinMode(ICSPDAT, INPUT);
    pinMode(PICMCLR, INPUT);
}

void senddata(uint32_t data, int bitcnt) {
    pinMode(ICSPDAT, OUTPUT);
    for (int i = 0; i < bitcnt; i++) {
        digitalWrite(ICSPCLK, HIGH);
        digitalWrite(ICSPDAT, data & 1);
        data >>= 1;
        digitalWrite(ICSPCLK, LOW);
    }
}

void incr(int cnt) {
    for (int i = 0; i < cnt; i++) {
        senddata(INCRADDRESS, 6);
        pc++;
        // Tdly 1 µs
        os_delay_us(1);
    }
}

void datacmd(uint16_t cmd, uint16_t data) {
    senddata(cmd, 6);
    // Tdly 1 µs
    os_delay_us(1);
    senddata(data << 1, 16);
}

uint16_t readcmd(uint16_t cmd) {
    uint16_t data = 0;
    senddata(cmd, 6);
    pinMode(ICSPDAT, INPUT);
    for (int i = 0; i < 16; i++) {
        digitalWrite(ICSPCLK, HIGH);
        data |= digitalRead(ICSPDAT) << i;
        digitalWrite(ICSPCLK, LOW);
    }
    // Discard the start and stop bits
    return data >> 1 & 0x3fff;
}

uint16_t start() {
    char key[] = KEY;

    digitalWrite(PICMCLR, LOW);
    pinMode(PICMCLR, OUTPUT);
    digitalWrite(PICMCLR, LOW);
    digitalWrite(ICSPCLK, LOW);
    pinMode(ICSPCLK, OUTPUT);
    digitalWrite(ICSPCLK, LOW);

    // Send 32-bit key, least significant byte first
    for (int i = 3; i >= 0; i--) {
        senddata(key[i], 8);
    }
    // Send one more dummy bit
    senddata(0, 1);

    // Load configuration
    datacmd(LOADCONFIG, 0);
    pc = 0x8000;
    incr(6);
    return readcmd(READPROGMEM);
}

uint16_t icsp_detect() {
    uint16_t id;
    id = start();
    // Texit = 1 µs
    os_delay_us(1);
    icsp_setup();
    return id;
}

void hexclose() {
    if (fd) fd.close();
    // Texit = 1 µs
    os_delay_us(1);
    icsp_setup();
    state = ICSPIDLE;
    mode = ICSPMODEIDLE;
}

icspresult icsp_program() {
    icsp_setup();
    fd = LittleFS.open("/pic16f1847/gateway.hex", "r");
    if (fd) {
        uint32_t id = start();
        if (id == 0x3fff || id == 0) {
            // No PIC found
            hexclose();
            return ICSP_NO_PIC;
        } else {
            debuglog(PSTR("PIC ID = 0x%x\n"), id);
            debuglog(PSTR("Erasing\n"));
            // Erase the device
            senddata(ERASEPROGMEM, 6);
            // Wait Terab (5 ms) for the operation to complete
            os_delay_us(5000);
            senddata(ERASEDATAMEM, 6);
            // Wait Terab (5 ms) for the operation to complete
            os_delay_us(5000);
            mode = ICSPMODEPROG;
            debuglog(PSTR("Programming\n"));
            hexseg = 0;
            return ICSP_PROGRAM;
        }
    } else {
        hexclose();
        return ICSP_NO_FILE;
    }
}

icspresult icsp_verify() {
    if (fd) {
        fd.seek(0, SeekSet);
    } else {
        fd = LittleFS.open("/pic16f1847/gateway.hex", "r");
        if (fd) {
            icsp_setup();
            uint32_t id = start();
            if (id == 0x3fff || id == 0) {
                hexclose();
                return ICSP_NO_PIC;
            }
        } else {
            hexclose();
            return ICSP_NO_FILE;
        }
    }
    debuglog(PSTR("Verifying\n"));
    mode = ICSPMODEVFY;
    hexseg = 0;
    errcnt = 0;
    linenum = 0;
    return ICSP_VERIFY;
}

void flush() {
    if (used) {
        senddata(PROGINTERNAL, 6);
        os_delay_us(2500);
        used = 0;
    }
}

icspresult icsp_step() {
    char buffer[128];
    int len, addr, tag, pos, cnt, cmd, byte, val;
    uint16_t hexaddr, vfy;
    uint8_t sum;
    union {
        uint8_t bytes[128];
        uint16_t words[64];
    } data;

    if (mode == ICSPMODEIDLE) return ICSP_FINISHED;
    cnt = fd.readBytesUntil('\n', buffer, sizeof(buffer));
    if (sscanf(buffer, ":%2x%4x%2x", &len, &addr, &tag) != 3) {
        hexclose();
        return ICSP_FORMAT_ERROR;
    }
    sum = 0 - len - (addr >> 8) - (addr & 0xff) - tag;
    pos = 9;
    for (int i = 0; i < len; i++) {
        if (sscanf(buffer + pos, "%2x", &byte) != 1) {
            hexclose();
            return ICSP_NO_DATA;
        }
        data.bytes[i] = byte;
        sum -= byte;
        pos += 2;
    }
    if (sscanf(buffer + pos, "%2x", &byte) != 1) {
        hexclose();
        return ICSP_NO_CHECKSUM;
    }
    if (sum != byte) {
        hexclose();
        return ICSP_CHECKSUM_ERROR;
    }
    linenum++;
    switch (tag) {
     case 0:
        // Data record
        hexaddr = (addr >> 1) + (hexseg << 3);
        if (hexaddr >= 0xf000) {
            // Data memory
            state = mode == ICSPMODEPROG ? ICSPPROGDATA : ICSPVFYDATA;
            hexaddr -= 0xf000;
            len >>= 1;
        } else if (hexaddr >= 0x8000) {
            if (state == ICSPPROGPROG) flush();
            if (state != ICSPPROGCONF && state != ICSPVFYCONF) {
                datacmd(LOADCONFIG, 0);
                pc = 0x8000;
                state = mode == ICSPMODEPROG ? ICSPPROGCONF : ICSPVFYCONF;
            }
            len >>= 1;
        } else {
            state = mode == ICSPMODEPROG ? ICSPPROGPROG : ICSPVFYPROG;
            len >>= 1;
        }
        if (pc > hexaddr) {
            // debuglog(PSTR("Reset address (%04x)\n"), pc);
            senddata(RESETADDRESS, 6);
            // Tdly 1 µs
            os_delay_us(1);
            pc = 0;
            row = 0;
        }
        for (int i = 0; i < len; i++) {
            if (state == ICSPPROGPROG) {
                uint16_t base = hexaddr & ~31;
                if (base != row) {
                    flush();
                    row = base;
                    // debuglog(PSTR("Row: %04x\n"), row);
                }
            }
            if (pc < hexaddr) {
                incr(hexaddr - pc);
            }
            switch (state) {
             case ICSPPROGPROG:
                if (i && (pc & 31) == 0) {
                    flush();
                    row += 32;
                    // debuglog(PSTR("Row: %04x\n"), row);
                }
                datacmd(LOADPROGMEM, data.words[i]);
                used++;
                break;
             case ICSPPROGCONF:
                datacmd(LOADPROGMEM, data.words[i]);
                senddata(PROGINTERNAL, 6);
                os_delay_us(5000);
                break;
             case ICSPPROGDATA:
                datacmd(LOADDATAMEM, data.words[i]);
                senddata(PROGINTERNAL, 6);
                os_delay_us(5000);
                break;
             case ICSPVFYPROG:
             case ICSPVFYCONF:
                vfy = readcmd(READPROGMEM);
                if (vfy != (data.words[i] & 0x3fff)) {
                    debuglog(PSTR("%04x (line %d): %04x -> %04x\n"),
                      pc, linenum, data.words[i] & 0x3fff, vfy);
                    if (errcnt++ >= 10) {
                        hexclose();
                        return ICSP_TOO_MANY_ERRORS;
                    }
                }
                break;
             case ICSPVFYDATA:
                vfy = readcmd(READDATAMEM);
                if (vfy != data.words[i]) {
                    debuglog(PSTR("%02x (line(%d): %04x -> %04x\n"),
                      pc, linenum, data.words[i], vfy);
                    if (errcnt++ >= 10) {
                        hexclose();
                        return ICSP_TOO_MANY_ERRORS;
                    }
                }
                break;
            }
            hexaddr++;
        }
        break;
     case 1:
        // End-of-file record
        if (mode == ICSPMODEPROG) {
            flush();
            // Tdis = 100 µs
            os_delay_us(100);
            return icsp_verify();
        } else {
            hexclose();
            return ICSP_FINISHED;
        }
        break;
     case 2:
        // Extended segment address record
        hexseg = data.bytes[1] | (data.bytes[0] << 8);
        break;
     case 4:
        // Extended linear address record
        hexseg = data.bytes[1] << 12;
        break;
     default:
        hexclose();
        return ICSP_UNKNOWN_TAG;
    }
    return ICSP_CONTINUE;
}

int icsperrors() {
    return errcnt;
}
