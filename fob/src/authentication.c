#include <stdint.h>
#include <stddef.h>
#include "authentication.h"
#include "bearssl_rand.h"

void issue_challenge(uint8_t* challenge) {

}

void solve_challenge(uint8_t* challenge, uint8_t* response) {

}

// Initialize BearSSL's pseudorandom number generator. This function must be called on boot.
//TODO: Define source of entropy and give it a seed
//Updates static pool with factory secret and random entropy
void init_random(void) {

    eeprom_init();

    uint8_t seed[SEED_SIZE];
    eeprom_read(seed, SEED_SIZE, EEPROM_RAND_ADDR);
    uint64_t e_factory = FACTORY_ENTROPY;
    br_hmac_drbg_init(&ctx, &br_sha256_vtable, &e_factory, 8);
    br_hmac_drbg_update(&ctx, seed, SEED_SIZE);

    br_sha256_context sha_ctx;
    br_sha256_init(&sha_ctx);
    br_sha256_update(&sha_ctx, entropy_sram, 2048);
    uint8_t hash_out[32];
    br_sha256_out(&sha_ctx, hash_out);

    br_hmac_drbg_update(&ctx, hash_out, 32);

    //override old source of persistent memory with new value
    br_hmac_drbg_generate(&ctx_rand, seed, SEED_SIZE);
    eeprom_write(seed, SEED_SIZE, EEPROM_RAND_ADDR);
}

void get_rand_bytes(uint8_t* out, size_t len) {
    uint8_t seed[SEED_SIZE];
    br_hmac_drbg_generate(&ctx, seed, SEED_SIZE);

<<<<<<< Updated upstream
    eeprom_write(seed, SEED_SIZE, EEPROM_RAND_ADDR);

    br_hmac_drbg_generate(&ctx, out, len);
=======
    br_hmac_drbg_generate(&ctx_rand, out, len);
>>>>>>> Stashed changes
}