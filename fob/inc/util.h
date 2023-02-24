/*
/* General Utilities for Device firmware
/* honestly this name is a bit misleading; it should be called datatypes.h
*/

#ifndef UTIL_H
#define UTIL_H

#define DEBUG 1

#include <stdint.h>
#include <stddef.h>

#define MESSAGE_HEADER_SIZE 83
#define PAYLOAD_BUF_SIZE 512

//message magics
#define TO_CAR 0x63   //('c')
#define TO_P_FOB 0x70 //('p')
#define TO_U_FOB 0x75 //('u')

//packet magics
#define HELLO 0x48 //('H')
#define CHALL 0x43 //('C')
#define SOLVE 0x53 //('R')
#define END 0x45   //('E')

#define UNLOCK_MGK 0x4F //('O')

typedef struct {
    uint8_t target;
    uint8_t msg_magic;
    uint64_t c_nonce;
    uint64_t s_nonce;
    size_t payload_size;
    uint8_t payload_buf[PAYLOAD_BUF_SIZE];
    uint8_t payload_hash[32];
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

//will compiler optimize this away?
//also isnt this just memset lol
#define safe_memset(address, value, size) \
    for(size_t i = 0; i < size; i++) { \
        if(i < size) {                 \
            ((uint8_t*) address)[i] = value;        \
        }                              \
    }                                  

#define debug_print(string) \
    uart_send_raw(HOST_UART, string, sizeof(string) - 1)
#endif
