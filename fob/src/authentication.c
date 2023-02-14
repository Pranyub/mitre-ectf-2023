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
void rand_init(void) {

    //initialize eeprom
    eeprom_init();

    //get persistent source of entropy
    uint8_t seed[SEED_SIZE];
    eeprom_read(seed, SEED_SIZE, EEPROM_RAND_ADDR);


    //get sram source of entropy
    br_sha256_context ctx_sha;
    uint8_t hash_out[32];
    br_sha256_init(&ctx_sha);
    br_sha256_update(&ctx_sha, entropy_sram, 2048);
    br_sha256_out(&ctx_sha, hash_out);

    //get factory source of entropy
    uint64_t e_factory = FACTORY_ENTROPY;

    //seed all entropy sources to random function
    br_hmac_drbg_init(&ctx_rand, &br_sha256_vtable, &e_factory, 8);
    br_hmac_drbg_update(&ctx_rand, seed, SEED_SIZE);
    br_hmac_drbg_update(&ctx_rand, hash_out, 32);

}

void rand_get_bytes(uint8_t* out, size_t len) {
    uint8_t seed[SEED_SIZE];
    br_hmac_drbg_generate(&ctx_rand, seed, SEED_SIZE);

    eeprom_write(seed, SEED_SIZE, EEPROM_RAND_ADDR);

    br_hmac_drbg_generate(&ctx_rand, out, len);
}