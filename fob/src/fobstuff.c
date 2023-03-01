#include "fobstuff.h"
#include "util.h"
#include "uart.h"
#include "authentication.h"
#include "secrets.h"
#include "inc/bearssl_hash.h"
#include "inc/bearssl_ec.h"


void handle_feature(void) {

    uint8_t packet[sizeof(Message)];

    if(!uart_read_message(HOST_UART, (Message* ) &packet)) {
        return;
    }


    if(packet[0] == UPLOAD_FEATURE) {
        uint8_t* feature
    }

}

void handle_upload_feature(uint8_t* packet) {

    uint8_t[32] hash;
    br_sha256_context ctx_sha;
    br_sha256_init(&ctx_sha);
    br_sha256_update(&ctx_sha, &packet[1], 32);
    br_sha256_out(&ctx_sha, hash);

    br_sha256_update(&ctx_sha_f, &cmd->feature_a.signature, sizeof(cmd->feature_a.signature));

    uint32_t verification = br_ecdsa_i15_vrfy_raw(&br_ec_p256_m15, hash, sizeof(hash), &factory_pub, &packet[33], 65);
}

