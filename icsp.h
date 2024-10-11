#include <Arduino.h>

typedef enum {
    ICSP_CONTINUE,
    ICSP_PROGRAM,
    ICSP_VERIFY,
    ICSP_FINISHED,
    ICSP_NO_PIC,
    ICSP_NO_FILE,
    ICSP_NO_DATA,
    ICSP_NO_CHECKSUM,
    ICSP_FORMAT_ERROR,
    ICSP_CHECKSUM_ERROR,
    ICSP_UNKNOWN_TAG,
    ICSP_TOO_MANY_ERRORS
} icspresult;

icspresult icsp_step();
uint16_t icsp_detect();
icspresult icsp_program();
icspresult icsp_verify();
int icsperrors();
