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

    if(target != P_FOB_TARGET) {
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

    if(next_packet_type != message->msg_magic) {
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


void parse_message(void) {

}

void send_response(void) {
    
}


/* Creates and sends a hello message as part of the Conversation Protocol

The hello message is the first stage of the Conversation Protocol.
It consists of a 32 byte random value that will later be used in the challenge solution

As of now, the creation of the packet and the sending of the packet occur in one function. Should this change?
*/
void gen_hello(void) {
    message_init(&current_msg);
    current_msg.msg_magic = HELLO;
    PacketHello p;
    rand_get_bytes(challenge, 32);
    memcpy(&p.chall, &challenge, 32); //is this safe?
    message_add_payload(&current_msg, &p, sizeof(p));
    next_packet_type = CHALL;
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
bool handle_chall(Message* message) {
    if(message->payload_size != sizeof(PacketChallenge)) {
        return false;
    }

    PacketChallenge* p = &(message->payload_buf);

    memcpy(challenge_resp, p->chall, sizeof(challenge_resp));

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