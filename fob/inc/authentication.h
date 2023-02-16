#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "uart.h"
#include "util.h"

//random should be stored on EEPROM so it may persist on reset
#define EEPROM_RAND_ADDR 0x0000

//address of uninitialized memory (use static memory directives in the future?)
#define RAND_UNINIT 0x00008000 - 2048

#define SEED_SIZE 32

//should be generated in 'secrets.h' and be unique for each fob
#define FACTORY_ENTROPY 0xdf013746886b5dcc
#define PAIR_SECRET {0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41}

//context for random number generator
static br_hmac_drbg_context ctx_rand;
static int is_random_set = 0;

//variables to be used in client_response messaging
static uint64_t c_nonce = 0;
static uint64_t s_nonce = 0;
static uint8_t challenge[32];
static uint8_t next_packet_type = 0; //type of packet expected to be recieved

void init_message(Message* out);

void send_hello(void);
void send_solution(Message* challenge);

void handle_chall(Message* message);

//reset state of packet handler
void reset_state(void);

//random functions
void rand_init(void);
void rand_get_bytes(void* out, size_t len);


#endif