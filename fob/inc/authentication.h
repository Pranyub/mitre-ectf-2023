#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include <stddef.h>
#include "bearssl_rand.h"
#include "uart.h"

//random should be stored on EEPROM so it may persist on reset
#define EEPROM_RAND_ADDR 0x0000
#define SEED_SIZE 32
//should be generated in 'secrets.h' and be unique for each fob
#define FACTORY_ENTROPY 0xdf013746886b5dcc

static br_hmac_drbg_context ctx_rand;

//get random sram bytes on load to be added to entropy (static?)
#define RAND_UNINIT 0x00008000 - 2048

void issue_challenge(uint8_t* challenge);
void solve_challenge(uint8_t* challenge, uint8_t* response);

void rand_init(void);
void rand_get_bytes(uint8_t* out, size_t len);

#endif