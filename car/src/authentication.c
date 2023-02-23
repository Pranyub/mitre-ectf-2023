#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "inc/bearssl_hash.h"
#include "inc/bearssl_hmac.h"
#include "inc/hw_memmap.h"
#include "authentication.h"
#include "uart.h"


//initialize message header values
void message_init(Message* out) {
    safe_memset(&current_msg, 0, sizeof(current_msg));
    out->target = target;
    out->c_nonce = c_nonce;
    out->s_nonce = s_nonce;
}

/* verify that a message is valid

Requirements:
    - client nonce must match stored client nonce
    - server nonce must match stored server nonce (unless no server nonce is present, in which case set it)
    - the recieved packet type matches the next expected packet type
    - the payload size is nonzero
    - the payload size matches the expected size for a specific payload type (TODO; maybe implement elsewhere?)
    - the hash of the payload matches the payload

    - if any of these checks are failed, returns false. Otherwise return true.
*/
bool verify_message(Message* message) {

    if(target != CAR_TARGET) {
        return false;
    }

    if(message->c_nonce != c_nonce) {
        return false;
    }
    
    if (message->msg_magic != CHALL && message->s_nonce != s_nonce)
    {
       return false;
    }

    if(message->payload_size < 1) {
        return false;
    }

    if(next_packet_type != message->msg_magic || next_packet_type == 0) {
        return false;
    }

    uint8_t hash[32];
    br_hmac_context ctx_hmac;
    br_hmac_init(&ctx_hmac, &ctx_hmac_key, sizeof(hash));
    br_hmac_update(&ctx_hmac, &message, sizeof(Message) - 512 + message->payload_size - sizeof(hash));
    br_hmac_outCT(&ctx_hmac, NULL, 0, 0, 0, hash);

    if(!timingsafe_memcmp(hash, message->payload_hash, sizeof(hash))) {
        return false;
    }

    return true;
}

// Adds a given message to a payload and computes the corresponding hash

void message_add_payload(Message* out, void* payload, size_t size) {
    if(size > PAYLOAD_BUF_SIZE) {
        return;
    }
    memcpy(out->payload_buf, payload, size);

    out->payload_size = size;
    br_hmac_context ctx_hmac;
    br_hmac_init(&ctx_hmac, &ctx_hmac_key, sizeof(out->payload_hash));
    br_hmac_update(&ctx_hmac, &out, sizeof(Message) - 512 + out->payload_size - sizeof(out->payload_hash));
    br_hmac_outCT(&ctx_hmac, NULL, 0, 0, 0, out->payload_hash);
}


void start_unlock_sequence(void) {
    reset_state();
    gen_hello();
    uart_send_message(DEVICE_UART, &current_msg);
    next_packet_type = CHALL;
}


void parse_inc_message(void) {
    uart_read_message(DEVICE_UART, &current_msg);
    if(!verify_message(&current_msg)) {
        uart_send_raw(HOST_UART, "msg verification fail\n", 23);
        reset_state();
        return;
    }

    switch (current_msg.msg_magic)
    {
    case HELLO:
        create_challenge();
        send_next_message();
        break;
    case END:
        handle_answer(&current_msg);
        break;
    default:
        uart_send_raw(HOST_UART, "bad magic\n", 11);
        reset_state();
        break;
    }
}

void send_next_message(void) {
    if(is_msg_ready) {
        is_msg_ready = false;
        uart_send_message(DEVICE_UART, &current_msg);
    }
}


// Car never says hi, we don't need to do this.

/*
Creates a challenge message via the Conversation Protocol.
This is the second stage of the convo protocol.
It consists of:
    - a 
*/
void create_challenge(){

}


/* Creates and sends a solution message as part of the Conversation Protocol (TODO)

The solution is the third stage of the Conversation Protocol.
It consists of:
    - a 32 byte response defined as sha256(hello_random, challenge_random, car_secret)
    - a Command payload; in this case an Unlock object
As of now, the creation of the packet and the sending of the packet occur in one function. Should this change?
*/

void gen_solution(void) {
    safe_memset(&current_msg, 0, sizeof(Message));
    message_init(&current_msg);
    current_msg.msg_magic = SOLVE;
    PacketSolution p;

    br_sha256_context ctx_sha2;
    br_sha256_init(&ctx_sha2);
    br_sha256_update(&ctx_sha2, challenge, sizeof(challenge));
    br_sha256_update(&ctx_sha2, challenge_resp, sizeof(challenge_resp));
    br_sha256_update(&ctx_sha2, car_secret, sizeof(car_secret));
    br_sha256_out(&ctx_sha2, p.response);

    p.command_magic = UNLOCK_MGK;    

    message_add_payload(&current_msg, &p, sizeof(p));

    next_packet_type = END;
    is_msg_ready = true;
}

/* Method to parse a challenge message as part of the Conversation Protocol

This parses and stores values pertaining to the second stage of the Conversation Protocol.

It stores the:
   - new server nonce
   - challenge bytes

As of now, the creation of the packet and the sending of the packet occur in one function. Should this change?
*/

bool handle_answer(Message* message) {
    uart_send_raw(HOST_UART, "recieved unlock!\n", 18);
    uart_send_message(HOST_UART, message);
    reset_state();
    return true;
}

/* Resets the internal state of Converstaion

Generates a new client nonce
Resets:
    - target type
    - server nonce
    - next packet type flag
    - challenge data
    - challenge resp data

*/
void reset_state(void) {

    rand_get_bytes(&c_nonce, sizeof(c_nonce));
    target = 0;
    s_nonce = 0;
    next_packet_type = 0;

    memset(challenge, 0, sizeof(challenge)); //safe?
    memset(challenge_resp, 0, sizeof(challenge_resp)); //safe?
}


// Initialize BearSSL's pseudorandom number generator. This function must be called on boot.
//TODO: Define source of entropy and give it a seed
//Updates static pool with factory secret and random entropy
void rand_init(void) {

    eeprom_init();

    uint8_t seed[SEED_SIZE];
    eeprom_read(seed, SEED_SIZE, EEPROM_RAND_ADDR);
    uint64_t e_factory = FACTORY_ENTROPY;
    br_hmac_drbg_init(&ctx_rand, &br_sha256_vtable, &e_factory, 8);
    br_hmac_drbg_update(&ctx_rand, seed, SEED_SIZE);

    br_sha256_context sha_ctx;
    br_sha256_init(&sha_ctx);
    br_sha256_update(&sha_ctx, (uint8_t*) RAND_UNINIT, 2048);
    uint8_t hash_out[32];
    br_sha256_out(&sha_ctx, hash_out);

    br_hmac_drbg_update(&ctx_rand, hash_out, 32);

    //override old source of persistent memory with new value
    br_hmac_drbg_generate(&ctx_rand, seed, SEED_SIZE);
    eeprom_write(seed, SEED_SIZE, EEPROM_RAND_ADDR);
    is_random_set = 1;


    //also init hmac while we're at it
    br_hmac_key_init(&ctx_hmac_key, &br_sha256_vtable, car_secret, sizeof(car_secret));

}

void rand_get_bytes(void* out, size_t len) {
    br_hmac_drbg_generate(&ctx_rand, out, len);
}