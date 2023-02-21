#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "inc/bearssl_hash.h"
#include "inc/hw_memmap.h"
#include "authentication.h"
#include "uart.h"


//initialize message header values
void message_init(Message* out) {
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

    if(next_packet_type != message->msg_magic) {
        return false;
    }

    uint8_t hash[32];
    br_sha256_context ctx_sha;
    br_sha256_init(&ctx_sha);
    br_sha256_update(&ctx_sha, message->payload, message->payload_size);
    br_sha256_update(&ctx_sha, car_secret, sizeof(car_secret));
    br_sha256_out(&ctx_sha, &hash);

    if(!timingsafe_memcmp(hash, message->payload, sizeof(hash))) {
        return false;
    }

    return true;
}

// Adds a given message to a payload and computes the corresponding hash

void message_add_payload(Message* out, void* payload, size_t size) {
    out->payload = payload;
    out->payload_size = size;
    br_sha256_context ctx_sha;
    br_sha256_init(&ctx_sha);
    br_sha256_update(&ctx_sha, payload, size);
    br_sha256_update(&ctx_sha, car_secret, sizeof(car_secret));
    br_sha256_out(&ctx_sha, &out->payload_hash);
}


void start_unlock_sequence(void) {
    reset_state();
    send_hello();
}


/* Creates and sends a hello message as part of the Conversation Protocol

The hello message is the first stage of the Conversation Protocol.
It consists of a 32 byte random value that will later be used in the challenge solution

As of now, the creation of the packet and the sending of the packet occur in one function. Should this change?
*/
void send_hello(void) {
    Message m;
    m.msg_magic = HELLO;
    message_init(&m);
    PacketHello p;
    rand_get_bytes(challenge, 32);
    memcpy(&p.chall, &challenge, 32); //is this safe?
    message_add_payload(&m, &p, sizeof(p));
    next_packet_type = CHALL;
    uart_send_message(HOST_UART, &m);
}


/* Creates and sends a solution message as part of the Conversation Protocol (TODO)

The solution is the third stage of the Conversation Protocol.
It consists of:
    - a 32 byte response defined as sha256(hello_random, challenge_random, car_secret)
    - a Command payload; in this case an Unlock object
As of now, the creation of the packet and the sending of the packet occur in one function. Should this change?
*/

void send_solution(void) {
    Message m;
    next_packet_type = END;
}

/* Method to parse a challenge message as part of the Conversation Protocol

This parses and stores values pertaining to the second stage of the Conversation Protocol.

It stores the:
   - new server nonce
   - challenge bytes

As of now, the creation of the packet and the sending of the packet occur in one function. Should this change?
*/
void handle_chall(Message* message) {
    s_nonce = message->s_nonce;
    //TODO: store challenge bytes
    //send_solution(message); //<-- should be elsewhere
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
}

void rand_get_bytes(void* out, size_t len) {
    br_hmac_drbg_generate(&ctx_rand, out, len);
}