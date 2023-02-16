#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#define MESSAGE_HEADER_SIZE 83

//message magics
#define HELLO 0x48 //('H')
#define CHALL 0x43 //('C')
#define RESP  0x53 //('R')

typedef struct {
    uint8_t msg_magic;
    size_t size;
    uint64_t c_nonce;
    uint64_t s_nonce;
    uint8_t* payload;
    uint8_t hash[32];
} Message;

typedef struct {
    uint8_t data[32];
    uint8_t signature[64];
} Feature;

typedef struct {
    uint8_t comm_magic;
    Feature feature_a;
    Feature feature_b;
    Feature feature_c;
    uint8_t signature_multi[64];

} Unlock;

typedef struct {
    uint8_t pak_magic;
    uint8_t chall[32];
} PacketHello;

typedef struct {
    uint8_t pak_magic;
    uint8_t chall[32];
} PacketChallenge;

typedef struct {
    uint8_t pak_magic;
    uint8_t response[32];
    Unlock command;
} PacketResponse;

#endif

