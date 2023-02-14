#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include "bearssl_rand.h"

static br_hmac_drbg_context ctx;

void issue_challenge(uint8_t* challenge);
void solve_challenge(uint8_t* challenge, uint8_t* response);

void init_random(void);
void get_rand_bytes(uint8_t* out, size_t len);

#endif