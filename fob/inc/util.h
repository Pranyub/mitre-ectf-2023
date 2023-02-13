#define UNIX 1
#include <stdint.h>

struct Message {
    uint8_t magic;
    uint16_t size;
    uint64_t c_nonce;
    uint64_t s_nonce;
    uint8_t challenge[32];
    uint8_t response[32];
    uint8_t* payload;
};

//void solve_challenge(uint8_t* challenge, uint8_t* response);