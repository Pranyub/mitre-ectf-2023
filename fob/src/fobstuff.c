#include "fobstuff.h"
#include "util.h"
#include "uart.h"
#include "authentication.h"
#include "secrets.h"
#include "inc/bearssl_hash.h"
#include "inc/bearssl_ec.h"


void handle_host_msg(void) {

    uint8_t packet[sizeof(Message)];

    if(!uart_read_message(HOST_UART, (Message* ) &packet)) {
        return;
    }


    if(packet[0] == UPLOAD_FEATURE) {
        handle_upload_feature(packet);
    }

}

void handle_upload_feature(uint8_t* packet) {

    uint8_t[32] hash;
    br_sha256_context ctx_sha;
    br_sha256_init(&ctx_sha);
    br_sha256_update(&ctx_sha, &packet[1], 32);
    br_sha256_out(&ctx_sha, hash);

    br_ec_public_key pk;
    pk.curve = BR_EC_brainpoolP256r1;
    pk.q = factory_pub;
    pk.qlen = 65;

    uint32_t verification = br_ecdsa_i15_vrfy_raw(&br_ec_p256_m15, hash, sizeof(hash), &pk, &packet[33], 64);

    #ifdef DEBUG
    debug_print("verification: ");
    uart_send_raw(HOST_UART, &verification, sizeof(verification));
    uart_send_raw(HOST_UART, &pakcet[1], 32);
    uart_send_raw(HOST_UART, &packet[33], 64);
    #endif
}

