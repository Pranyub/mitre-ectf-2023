#include <stdint.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "inc/bearssl_hash.h"
#include "inc/hw_memmap.h"
#include "authentication.h"
#include "uart.h"

void message_init(Message* out) {
    out->msg_magic = CAR_TARGET;
    if(!c_nonce)
        rand_get_bytes(&c_nonce, sizeof(c_nonce));
    out->c_nonce = c_nonce;
    out->s_nonce = s_nonce;
}

void message_add_payload(Message* out, void* payload, size_t size) {
    out->payload = payload;
    out->payload_size = size;
    br_sha256_context ctx_sha;
    br_sha256_init(&ctx_sha);
    br_sha256_update(&ctx_sha, payload, size);
    br_sha256_out(&ctx_sha, &out->payload_hash);
}

void send_hello() {
    Message m;
    message_init(&m);
    PacketHello p;
    p.pak_magic = HELLO;
    rand_get_bytes(challenge, 32);
    memcpy(&p.chall, &challenge, 32); //is this side channel resistant?
    message_add_payload(&m, &p, sizeof(p));
    uart_send_message(HOST_UART, &m);
}


void solve_challenge(uint8_t* challenge, uint8_t* response) {

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