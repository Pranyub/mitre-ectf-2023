#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "inc/bearssl_hmac.h"
#include "uart.h"
#include "util.h"

//random should be stored on EEPROM so it may persist on reset
#define EEPROM_RAND_ADDR 0x0000

//address of uninitialized memory (use static memory directives in the future?)
#define RAND_UNINIT 0x00008000 - 2048

#define SEED_SIZE 32

//should be generated in 'secrets.h' and be unique for each fob (make this uint8[32])
#define FACTORY_ENTROPY 0xa4b309d3c49103f3
#define PAIR_SECRET {0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41}
static uint8_t car_secret[] = PAIR_SECRET; //is there a better way to do this?

//context for random number generator
static br_hmac_drbg_context ctx_rand;
static br_hmac_key_context ctx_hmac_key;
static int is_random_set = 0; //bool to make sure random is set

/******************************************************************/
//        variables to be used in conversation messaging
/******************************************************************/
static uint8_t target = 0;
static uint64_t c_nonce = 0;
static uint64_t s_nonce = 0;

static uint8_t challenge[32];
static uint8_t challenge_resp[32];
static uint8_t next_packet_type = 0; //type of packet expected to be recieved

// All functions creating / modifying a message will use this variable;
// It's probably more time-efficient than creating a new message struct every single time
static Message current_msg;
static bool is_msg_ready = false;
/******************************************************************/

/******************************************************************/


//initializes a message struct with header values
void init_message(Message* out);

//resest the state of packet handler
void reset_state(void);

//authenticates a message
bool verify_message(Message* message);

void parse_inc_message(void);
void send_next_message(void);

bool handle_answer(Message* message);

//random functions
void rand_init(void);
void rand_get_bytes(void* out, size_t len);

#endif