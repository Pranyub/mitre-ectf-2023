#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "inc/bearssl_hash.h"
#include "inc/hw_memmap.h"
#include "authentication.h"
#include "uart.h"

void message_init(Message* out) {
    out->target = CAR_TARGET;
    out->c_nonce = c_nonce;
    out->s_nonce = s_nonce;
}


bool verify_message(Message* message) {

    if(message->c_nonce != c_nonce) {
        return false;
    }

    if(next_packet_type == CHALL) {
        s_nonce = message->s_nonce;
    }
    else if (message->s_nonce != s_nonce)
    {
       return false;
    }

    if(message->payload_size < 1) {
        return false;
    }

    if(next_packet_type != ((uint8_t*)message->payload)[0]) {
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

void message_add_payload(Message* out, void* payload, size_t size) {
    out->payload = payload;
    out->payload_size = size;
    br_sha256_context ctx_sha;
    br_sha256_init(&ctx_sha);
    br_sha256_update(&ctx_sha, payload, size);
    br_sha256_update(&ctx_sha, car_secret, sizeof(car_secret));
    br_sha256_out(&ctx_sha, &out->payload_hash);
}

void send_hello(void) {
    reset_state();
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


void send_solution(Message* challenge) {
    Message m;
    m.msg_magic = CAR_TARGET;
    m.c_nonce = c_nonce;
    m.s_nonce = s_nonce;
    next_packet_type = END;
}

void handle_chall(Message* message) {
    s_nonce = message->s_nonce;
    send_solution(message);
}

void reset_state(void) {

    rand_get_bytes(&c_nonce, sizeof(c_nonce));

    s_nonce = 0;
    next_packet_type = 0;

    memset(challenge, 0, sizeof(challenge));
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