// Copyright (c) 2021 - Schelte Bron

#include "data.h"
#include "webserver.h"
#include "web.h"
#include "debug.h"
#include <sys/time.h>

typedef union {
  unsigned raw;
  struct {
    unsigned int value: 16;
    unsigned int dataid: 8;
    unsigned int spare: 4;
    unsigned int msgtype: 3;
    unsigned int parity: 1;
  } frame;
  struct {
    unsigned int minutes: 8;
    unsigned int hours: 5;
    unsigned int weekday: 3;
  } time;
  struct {
    unsigned int lb: 8;
    unsigned int hb: 8;
  } bytes;
} otmessage;

struct {
  unsigned int timestamp;
  unsigned int message;
} history[3600];

unsigned errorcnt[4];
static unsigned short store[64];
static uint64_t storemap[1];

int otflags(char *s, unsigned val, int cnt = 8) {
  int mask;

  for (mask = 1 << (cnt - 1); mask > 0; mask >>= 1) {
    *s++ = '0' + ((val & mask) != 0);
  }
  *s = '\0';
  return cnt;
}

int otformat(char *buf, char dir, unsigned raw) {
  otmessage msg;
  const char *msgptr = nullptr, *pmemptr;
  char *s = buf;
  int fmt = OTFORMATNONE;
  timeval now;
  struct tm *tod;

  msg.raw = raw;

  gettimeofday(&now, nullptr);
  tod = localtime(&now.tv_sec);
  s += sprintf(s, "%02d:%02d:%02d.%06d  ", tod->tm_hour, tod->tm_min, tod->tm_sec, (int)now.tv_usec);
  
  pmemptr = msgtypes[msg.frame.msgtype];
  s += sprintf(s, "%c%08X  ", dir, msg.raw);
  strcpy_P(s, pmemptr);
  s += strlen(s);
  sprintf(s, "    ");
  s = buf + 40;
  if (msg.frame.dataid < 128) {
    msgptr = msgids[msg.frame.dataid];
  }
  if (msgptr) {
    strcpy_P(s, msgptr);
    s += strlen(s);
    fmt = pgm_read_byte(msgfmts + msg.frame.dataid);
  } else {
    s += sprintf(s, "Message ID %d", msg.frame.dataid);
  }
  s += sprintf(s, ": ");

  switch (fmt) {
    case OTFORMATDATE:
      pmemptr = datetimestr + (msg.bytes.hb + 8) * 4;
      strcpy_P(s, pmemptr);
      s += 3;
      s += sprintf(s, " %d", msg.bytes.lb);
      break;
    case OTFORMATTIME:
      pmemptr = datetimestr + msg.time.weekday * 4;
      strcpy_P(s, pmemptr);
      s += 3;
      s += sprintf(s, " %02d:%02d", msg.time.hours, msg.time.minutes);
      break;
    case OTFORMATRFSENSOR:
      s += sprintf(s, "%d %d %d %d", msg.bytes.hb & 0xf, msg.bytes.hb >> 4, msg.bytes.lb & 0x3, msg.bytes.lb >> 2 & 0x7);
      break;
    case OTFORMATOVERRIDE:
      s += sprintf(s, "%d %d %d ", msg.bytes.hb & 0xf, msg.bytes.hb >> 4, msg.bytes.lb & 0xf);
      s += otflags(s, msg.bytes.lb >> 4, 4);
      break;
    case OTFORMATFLOAT:
      s += sprintf(s, "%.2f", (short)msg.frame.value / 256.);
      break;
    case OTFORMATFLAGFLAG:
      s += otflags(s, msg.bytes.hb);
      *s++ = ' ';
      s += otflags(s, msg.bytes.lb);
      break;
    case OTFORMATFLAGUBYTE:
      s += otflags(s, msg.bytes.hb);
      s += sprintf(s, " %d", msg.bytes.lb);
      break;
    case OTFORMATFLAGLB:
      s += otflags(s, msg.bytes.lb);
      break;
    case OTFORMATUBYTELB:
      s += sprintf(s, "%d", msg.bytes.lb);
      break;
    case OTFORMATINTEGER:
      s += sprintf(s, "%d", (short)msg.frame.value);
      break;
    case OTFORMATBYTEBYTE:
      s += sprintf(s, "%d %d", (char)msg.bytes.hb, (char)msg.bytes.lb);
      break;
    case OTFORMATUBYTEHB:
      s += sprintf(s, "%d", msg.bytes.hb);
      break;
    case OTFORMATUBYTEUBYTE:
      s += sprintf(s, "%d %d", msg.bytes.hb, msg.bytes.lb);
      break;
    case OTFORMATUNSIGNED:
    default:
      s += sprintf(s, "%d", msg.frame.value);
  }
  return s - buf;
}

int bitflags(char *s, byte id, unsigned short value, unsigned short mask) {
  int i, n = 0;
  for (int i = 0; mask != 0; i++, mask >>= 1) {
    if (mask & 1) {
      n += sprintf(s + n, "\"msgid%d%cB%d\":%d,", id, i > 7 ? 'H' : 'L', i & 7, (value >> i) & 1);
    }
  }
  return n;
}

void otstatus(unsigned raw) {
  otmessage msg;
  char *s, jsonbuf[128];
  bool report = false;
  unsigned short change, mask;

  msg.raw = raw;
  
  switch (msg.frame.dataid) {
    case 0:
      change = msg.frame.value ^ store[0];
      // If no previous information was known, report everything
      mask = bitRead(storemap[0], 0) ? change : 0x1fff;
      if (msg.frame.msgtype == 0) {
        mask &= 0xff00;
      } else if (msg.frame.msgtype == 4) {
        mask &= 0x00ff;
        // Only mark as known when a response was seen
        bitSet(storemap[0], 0);
      } else {
        break;
      }
      if (mask == 0) break;
      store[0] ^= change & mask;
      s = jsonbuf;
      *s = '{';
      s += bitflags(s + 1, 0, msg.frame.value, mask);
      *s++ = '}';
      *s = '\0';
      websockreport(jsonbuf);
      break;
    case 5:
      if (msg.frame.msgtype != 4) break;
      change = msg.frame.value ^ store[5];
      // If no previous information was known, report everything
      mask = bitRead(storemap[0], 5) ? change : 0x3fff;
      if (mask == 0) break;
      store[5] ^= change & mask;
      bitSet(storemap[0], 5);
      s = jsonbuf;
      *s = '{';
      if (mask & 0xff00) {
        s += bitflags(s + 1, 5, msg.frame.value, mask & 0xff00);
      }
      if (mask & 0xff) {
        s += sprintf(s + 1, "\"msgid%dLB\":%d,", 5, msg.frame.value & 0xff);
      }
      *s++ = '}';
      *s = '\0';
      websockreport(jsonbuf);
      break;
    case 1:   // controlsp
    case 8:   // controlsp2
    case 14:  // maxmod
    case 16:  // setpoint
    case 23:  // setpoint2
    case 24:  // roomtemp
    case 37:  // roomtemp2
      report = report || msg.frame.msgtype == 1;
      if (!report) break;
    case 9:   // override
    case 17:  // modulation
    case 25:  // boilertemp
    case 26:  // dhwtemp
    case 28:  // returntemp
    case 31:  // boilertemp2
    case 32:  // dhwtemp2
    case 56:  // dhwsetpoint
    case 57:  // chwsetpoint
      report = report || msg.frame.msgtype == 4;
      if (!report) break;
    case 27:  // outside
      if (report || msg.frame.msgtype == 1 || msg.frame.msgtype == 4) {
        unsigned short value;
        value = msg.frame.value ? msg.frame.value : 1;
        if (value != store[msg.frame.dataid]) {
          store[msg.frame.dataid] = value;
          bitSet(storemap[msg.frame.dataid / 64], msg.frame.dataid % 64);
          // Transfer numbers as strings to keep the formatting
          sprintf(jsonbuf, "{\"msgid%d\":\"%.2f\"}", msg.frame.dataid, (short)msg.frame.value / 256.);
          websockreport(jsonbuf);
        }
      }
      break;
    default:
      break;
  }
}

void oterror(int num) {
  char jsonbuffer[28];
  if (num > 0 && num <= 4) {
    sprintf(jsonbuffer, "{\"error%d\":%d}", num, ++errorcnt[num - 1]);
    websockreport(jsonbuffer);
  }
}

char *initialreport(int num) {
  char jsonbuf[MAX_PAYLOAD_SIZE];
  int i, j, cnt = 0;
  unsigned short mask;

  jsonbuf[0] = '{';
  for (i = 0; i < 64; i++) {
    if (bitRead(storemap[i / 64], i % 64) == 0) continue;
    switch (i) {
     case 0:    // status
      cnt += bitflags(jsonbuf + cnt + 1, i, store[i], 0x1fff);
      break;
     case 5:    // asfflags
      cnt += bitflags(jsonbuf + cnt + 1, i, store[i], 0x3f00);
      cnt += sprintf(jsonbuf + cnt + 1, "\"msgid%dLB\":%d,", i, store[i] & 0xff);
      break;
     case 1:    // controlsp
     case 8:    // controlsp2
     case 9:    // override
     case 14:   // maxmod
     case 16:   // setpoint
     case 17:   // modulation
     case 23:   // setpoint2
     case 24:   // roomtemp
     case 25:   // boilertemp
     case 26:   // dhwtemp
     case 27:   // outside
     case 28:   // returntemp
     case 31:   // boilertemp2
     case 32:   // dhwtemp2
     case 37:   // roomtemp2
     case 56:   // dhwsetpoint
     case 57:   // chwsetpoint
      cnt += sprintf(jsonbuf + cnt + 1, "\"msgid%d\":\"%.2f\",", i, (short)store[i] / 256.);
      break;
     default:
      break;
    }
    if (cnt >= MAX_PAYLOAD_SIZE - 50) {
      jsonbuf[cnt] = '}';
      websocketsend(num, jsonbuf);
      cnt = 0;
    }
  }
  for (i = 0; i < 4; i++) {
    cnt += sprintf(jsonbuf + cnt + 1, "\"error%d\":%d,", i + 1, errorcnt[i]);
  }
  jsonbuf[cnt] = '}';
  websocketsend(num, jsonbuf);
}
