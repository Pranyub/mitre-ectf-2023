#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "uart.h"
#include "util.h"

//random should be stored on EEPROM so it may persist on reset
#define EEPROM_RAND_ADDR 0x0000
#define SEED_SIZE 32
//should be generated in 'secrets.h' and be unique for each fob
#define FACTORY_ENTROPY 0xdf013746886b5dcc

//context for random number generator
static br_hmac_drbg_context ctx_rand;
static int is_random_set = 0;

//
static uint64_t c_nonce = 0;
static uint64_t s_nonce = 0;
static uint8_t challenge[32];
static uint8_t stage = 0;

//get random sram bytes on load to be added to entropy (static?)
#define RAND_UNINIT 0x00008000 - 2048


void init_message(Message* out);
void send_hello();
void solve_challenge(uint8_t* challenge, uint8_t* response);

void rand_init(void);
void rand_get_bytes(void* out, size_t len);

#endif