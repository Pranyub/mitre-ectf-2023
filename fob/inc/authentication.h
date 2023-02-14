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

static br_hmac_drbg_context ctx;

//get random sram bytes on load to be added to entropy (static?)
volatile uint8_t entropy_sram[2048] __attribute__((section (".noinit")));

void issue_challenge(uint8_t* challenge);
void solve_challenge(uint8_t* challenge, uint8_t* response);

void init_random(void);
void get_rand_bytes(uint8_t* out, size_t len);

#endif