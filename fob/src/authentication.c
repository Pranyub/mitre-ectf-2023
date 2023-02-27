#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "inc/bearssl_hash.h"
#include "inc/bearssl_hmac.h"
#include "inc/hw_memmap.h"
#include "authentication.h"
#include "uart.h"


/**
 * @brief Zeroes a message and sets the nonces
 * 
 * @param message the message to be initialized
 */
void init_message(Message* message) {
    safe_memset(&current_msg, 0, sizeof(current_msg));
    message->c_nonce = c_nonce;
    message->s_nonce = s_nonce;
}


/**
 * @brief Verifies that a given message is authenticated and valid.
 * 
 * @return true if all of the following conditions are met:
 *  - message target matches device type
 *  - client and server nonces are valid
 *  - the message type is expected
 *  - the hmac of the message is valid
 * @return false otherwise
 * 
 * @param message the message to be verified
 */
bool verify_message(Message* message) {

    // Ensure message is going to the correct device type
    if(message->target != DEVICE_TYPE) {
        return false;
    }

    // Ensure the client nonce is valid if it needs to be
    if(message->msg_magic != HELLO && message->c_nonce != c_nonce) {
        return false;
    }
    
    // Ensure the server nonce is valid if it needs to be
    if (message->msg_magic != HELLO && message->msg_magic != CHALL && message->s_nonce != s_nonce)
    {
       return false;
    }

    // Redundant(?) check to ensure there is a payload
    if(message->payload_size < 1) {
        return false;
    }

    // Ensure that the recieved packet type is the expected type of packet to be recieved
    if(next_packet_type != message->msg_magic || next_packet_type == 0) {
        return false;
    }

    // Ensure that the hmac of the packet is valid
    uint8_t hash[32];
    br_hmac_context ctx_hmac;
    br_hmac_init(&ctx_hmac, &ctx_hmac_key, sizeof(hash));
    br_hmac_update(&ctx_hmac, &message, sizeof(Message) - PAYLOAD_BUF_SIZE \
        + message->payload_size - sizeof(hash));

    br_hmac_outCT(&ctx_hmac, NULL, 0, 0, 0, hash);

    if(!timingsafe_memcmp(hash, message->payload_hash, sizeof(hash))) {
        return false;
    }

    return true;
}

/**
 * @brief Finalizes the addition of a payload to a message by signing it and adding the hash & size fields
 * 
 * @param message the message to be signed
 * @param size the size of the message's payload
 */
void message_sign_payload(Message* message, size_t size) {

    //make sure the size isn't too large
    if(size > PAYLOAD_BUF_SIZE) {
        return;
    }

    //set the size
    message->payload_size = size;

    br_hmac_context ctx_hmac;
    br_hmac_init(&ctx_hmac, &ctx_hmac_key, sizeof(message->payload_hash));
    br_hmac_update(&ctx_hmac, &message, sizeof(Message) - PAYLOAD_BUF_SIZE \
        + message->payload_size - sizeof(message->payload_hash));

    br_hmac_outCT(&ctx_hmac, NULL, 0, 0, 0, message->payload_hash);
}

/**
 * @brief Attempts to read a message packet from UART and parses it
 * 
 * This is where the core Conversation logic occurs - the function
 * verifies that the incoming message is authenticated and valid and
 * parses the data accordingly.
 * 
 * It then attempts to craft a response to the incoming packet
 * 
 * If the incoming packet is invalid, the internal state of the current conversation
 * is reset.
 */

void parse_inc_message(void) {
    uart_read_message(DEVICE_UART, &current_ct, &current_msg);
    decrypt_message(&current_ct); // hopefully this works lol
    //remove second verification if slow
    if(!verify_message(&current_msg) && !verify_message(&current_msg)) {
        safe_memset(&current_msg, 0, sizeof(Message));
        uart_send_raw(HOST_UART, "msg verification fail\n", 23);
        reset_state();
        return;
    }

    switch (current_msg.msg_magic)
    {
    #ifdef FOB_TARGET
    case CHALL:
        if(handle_chall(&current_msg)) {
            gen_solution();
            break;
        }
    case END:
        handle_end(&current_msg);
        reset_state();
        break;
    #endif

    #ifdef CAR_TARGET
    case HELLO:
        if(handle_hello(&current_msg)) {
            gen_chall();
            break;
        }
    case SOLVE:
        if(handle_solution(&current_msg)) {
            gen_end();
            break;
        }
    #endif
    default:
        uart_send_raw(HOST_UART, "bad packet\n", 11);
        reset_state();
        break;
    }
}

void send_next_message(void) {
    if(is_msg_ready) {
        is_msg_ready = false;
        uart_send_message(DEVICE_UART, &current_ct);
    }
}

// Takes a given message and encrypts it, adds length as well
void encrypt_message(Message* pt) {
    current_ct.target = pt->target;
    current_ct.msg_magic = pt->msg_magic;
    rand_get_bytes(&current_ct.nonce, 12);

    br_aes_ct_ctr_keys aes_ctx;
    br_aes_ct_ctr_init(&aes_ctx, &key, 32);
    // we want to encrypt everything except the target and magic
    br_aes_ct_ctr_run(&aes_ctx, &current_ct.nonce, 0, &(pt->c_nonce), sizeof(Message) - 2);

    current_ct.msg = pt; // encryption done in-place so just point to message
}

// Takes a given message, parses it, and decrypts it
void decrypt_message(EncryptedMessage* ct) {
    current_msg.target = ct->target;
    current_msg.msg_magic = ct->msg_magic;

    br_aes_ct_ctr_keys aes_ctx;
    br_aes_ct_ctr_init(&aes_ctx, &key, 32);
    // we want to decrypt everything in place except the target and magic
    br_aes_ct_ctr_run(&aes_ctx, ct->nonce, 0, ct->msg, sizeof(Message) - 2);
}

/************************************************************************************
 * The following section of functions is reserved only for use with a fob fimrware. *
 * A car does not require these functions.                                          *
 ************************************************************************************/

#ifdef FOB_TARGET

/**
 * @brief Starts a conversation with a car to unlock the fob
 * 
 * This function should be called when the unlock button is pressed on a fob.
 * It resets the internal state of the current active conversation and
 * sends a new hello message through the device UART.
 */
void start_unlock_sequence(void) {
    reset_state();
    gen_hello();
    uart_send_message(DEVICE_UART, &current_ct);
    next_packet_type = CHALL;
}

/* Creates and sends a hello message as part of the Conversation Protocol
The hello message is the first stage of the Conversation Protocol.
It consists of a 32 byte random value that will later be used in the challenge solution
As of now, the creation of the packet and the sending of the packet occur in one function.
Should this change?
*/
void gen_hello(void) {
    safe_memset(&current_msg, 0, sizeof(Message));
    init_message(&current_msg);
    current_msg.msg_magic = HELLO;
    current_msg.target = TO_CAR;
    PacketHello* p = (PacketHello*) &current_msg.payload_buf;
    rand_get_bytes(challenge, sizeof(challenge));
    memcpy(&p->chall, &challenge, sizeof(challenge)); //is this safe?
    message_sign_payload(&current_msg, sizeof(p));
    next_packet_type = CHALL;
    encrypt_message(&current_msg);
    is_msg_ready = true;
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
    init_message(&current_msg);
    current_msg.msg_magic = SOLVE;
    current_msg.target = TO_CAR;
    PacketSolution* p = (PacketSolution*) &current_msg.payload_buf;


    // use hmac here instead?
    br_sha256_context ctx_sha2;
    br_sha256_init(&ctx_sha2);
    br_sha256_update(&ctx_sha2, challenge, sizeof(challenge));
    br_sha256_update(&ctx_sha2, challenge_resp, sizeof(challenge_resp));
    br_sha256_update(&ctx_sha2, car_secret, sizeof(car_secret));
    br_sha256_update(&ctx_sha2, c_nonce, sizeof(c_nonce));
    br_sha256_update(&ctx_sha2, s_nonce, sizeof(s_nonce));
    br_sha256_out(&ctx_sha2, p->response);

    p->command_magic = UNLOCK_MGK;    

    message_sign_payload(&current_msg, sizeof(p));
    encrypt_message(&current_msg);

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
bool handle_chall(Message* message) {
    if(message->payload_size != sizeof(PacketChallenge)) {
        return false;
    }

    PacketChallenge* p = &(message->payload_buf);

    memcpy(challenge_resp, p->chall, sizeof(challenge_resp));

    return true;
}

bool handle_end(Message* message) {

    uart_send_raw(HOST_UART, "recieved unlock!\n", 18);
    encrypt_message(message);
    uart_send_message(HOST_UART, &current_ct);
    reset_state();
    return true;
}
#endif


/************************************************************************************
 * The following section of functions is reserved only for use with a car fimrware. *
 * A fob (paired or unpaired) does not require these functions.                     *
 ************************************************************************************/

#ifdef CAR_TARGET

bool handle_hello(Message* message) {

    if(message->payload_size != sizeof(PacketHello)) {
        return false;
    }

    c_nonce = message->c_nonce;
    rand_get_bytes(&s_nonce, sizeof(s_nonce));

    PacketHello* p = (PacketHello*) &message->payload_buf;

    memcpy(challenge, p->chall, sizeof(challenge));

    return true;
}

bool handle_solution(Message* message) {
    if(message->payload_size != sizeof(PacketSolution)) {
        return false;
    }
    PacketSolution* p = (PacketSolution*) &message->payload_buf;
    
    uint8_t auth_hash[32];

    br_sha256_context ctx_sha2;
    br_sha256_init(&ctx_sha2);
    br_sha256_update(&ctx_sha2, challenge, sizeof(challenge));
    br_sha256_update(&ctx_sha2, challenge_resp, sizeof(challenge_resp));
    br_sha256_update(&ctx_sha2, car_secret, sizeof(car_secret));
    br_sha256_update(&ctx_sha2, c_nonce, sizeof(c_nonce));
    br_sha256_update(&ctx_sha2, s_nonce, sizeof(s_nonce));
    br_sha256_out(&ctx_sha2, auth_hash);

    return timingsafe_memcmp(auth_hash, p->response);
}

void gen_chall(void) {
    safe_memset(&current_msg, 0, sizeof(Message));
    init_message(&current_msg);
    current_msg.msg_magic = CHALL;
    current_msg.target = TO_P_FOB;

    PacketChallenge* p = (PacketChallenge*) &current_msg.payload_buf;
    rand_get_bytes(challenge, sizeof(challenge));
    memcpy(challenge, p->chall, sizeof(challenge));
    encrypt_message(&current_msg);

    next_packet_type = SOLVE;
    is_msg_ready = true;
}

void gen_end(void) {
    safe_memset(&current_msg, 0, sizeof(Message));
    init_message(&current_msg);
    current_msg.msg_magic = END;
    current_msg.target = TO_P_FOB;

    memcpy(current_msg.payload_buf, "unlocked! feature 1: ...\nfeature 2: ...\nfeature3: ...\n", 54);
    encrypt_message(&current_msg);

    next_packet_type = HELLO;
    is_msg_ready = true;
}
#endif

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
    rand_get_bytes(&s_nonce, sizeof(s_nonce));

    next_packet_type = 0;

    memset(challenge, 0, sizeof(challenge)); //safe?
    memset(challenge_resp, 0, sizeof(challenge_resp)); //safe?
}


/**
 * Function that initializes the pseudorandom generator with entropy from the following sources:
 *  - factory seed
 *  - persistent mutating code stored in eeprom
 *  - random bits from uninitialized section of ram
 *  - temperature (TODO)
 * 
 * Upon initialization, it overrides the stored seed in eeprom with a different value.
 * Also initializes HMAC context
 */
void rand_init(void) {

    eeprom_init();

    // Update rand with factory seed
    uint64_t e_factory = FACTORY_ENTROPY;
    br_hmac_drbg_init(&ctx_rand, &br_sha256_vtable, &e_factory, 8);

    // Update rand with EEPROM seed
    uint8_t seed[SEED_SIZE];
    eeprom_read(seed, SEED_SIZE, EEPROM_RAND_ADDR);
    br_hmac_drbg_update(&ctx_rand, seed, SEED_SIZE);

    // Update rand with uninitialized ram
    br_sha256_context sha_ctx;
    br_sha256_init(&sha_ctx);
    br_sha256_update(&sha_ctx, (uint8_t*) RAND_UNINIT, 2048);
    uint8_t hash_out[32];
    br_sha256_out(&sha_ctx, hash_out);
    br_hmac_drbg_update(&ctx_rand, hash_out, 32);

    //TODO: Update rand with temperature
    //

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