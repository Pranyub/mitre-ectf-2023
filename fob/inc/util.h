/*
/* General Utilities for Device firmware
/* honestly this name is a bit misleading; it should be called datatypes.h
*/

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

#define MESSAGE_HEADER_SIZE 83

//message magics
#define CAR_TARGET 0x63
#define P_FOB_TARGET 0x70
#define U_FOB_TARGET 0x75

//packet magics
#define HELLO 0x48 //('H')
#define CHALL 0x43 //('C')
#define SOLVE 0x53 //('R')
#define END 0x45   //('E')

typedef struct {
    uint8_t target;
    uint8_t msg_magic;
    uint64_t c_nonce;
    uint64_t s_nonce;
    uint8_t payload_hash[32];
    size_t payload_size;
    void* payload;
} Message;

typedef struct {
    uint8_t data[32];
    uint8_t signature[64];
} Feature;

typedef struct {
    Feature feature_a;
    Feature feature_b;
    Feature feature_c;
    uint8_t signature_multi[64];

} CommandUnlock;

typedef struct {
    uint8_t chall[32];
} PacketHello;

typedef struct {
    uint8_t chall[32];
} PacketChallenge;

typedef struct {
    uint8_t command_magic;
    size_t command_length;
    uint8_t response[32];
    uint8_t command[352];
} PacketSolution;

#endif

