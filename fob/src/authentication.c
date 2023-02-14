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
    br_hmac_drbg_init(&ctx, &br_sha256_vtable, NULL, 0);
}

void get_rand_bytes(uint8_t* out, size_t len) {
    br_hmac_drbg_generate(&ctx, out, len);
}