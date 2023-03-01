#ifndef FOBSTUFF_H
#define FOBSTUFF_H
#include <stdbool.h>
#include <stdint.h>
#include "secrets.h"

#define UPLOAD_FEATURE 0x1a
#define UPLOAD_SIG 0x2b
#define PAIR_CMD 0x3c
#define BOARD_PAIR 0x4d

typedef struct {
    uint8_t data[200];
} HostPacket;



static bool paired = PAIRED

void handle_feature(void);

#endif