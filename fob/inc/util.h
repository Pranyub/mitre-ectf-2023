#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#define MESSAGE_HEADER_SIZE 83

typedef struct {
    uint8_t magic;
    uint16_t size;
    uint64_t c_nonce;
    uint64_t s_nonce;
    uint8_t* payload;
    uint8_t hash[32];
} Message;

typedef struct {
    uint8_t magic;
    uint8_t chall[32];
} Challenge;

#endif

