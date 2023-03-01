#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "inc/bearssl_rand.h"
#include "inc/bearssl_hash.h"
#include "inc/bearssl_hmac.h"
#include "inc/bearssl_ec.h"
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
    #ifdef CAR_TARGET
    message->target = TO_P_FOB;
    #endif
    #ifdef P_FOB_TARGET
    message->target = TO_CAR;
    #endif
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

        #ifdef DEBUG
        debug_print("bad target: ");
        uart_send_raw(HOST_UART, &message->target, sizeof(message->target));
        #endif

        return false;
    }

    // Ensure the client nonce is valid if it needs to be
    if(message->msg_magic != HELLO && message->c_nonce != c_nonce) {

        #ifdef DEBUG
        debug_print("bad c_nonce: ");
        uart_send_raw(HOST_UART, &message->c_nonce, sizeof(message->c_nonce));
        uart_send_raw(HOST_UART, &c_nonce, sizeof(c_nonce));
        #endif

        return false;
    }
    
    // Ensure the server nonce is valid if it needs to be
    if (message->msg_magic != HELLO && message->msg_magic != CHALL && message->s_nonce != s_nonce)
    {
        #ifdef DEBUG
        debug_print("bad s_nonce: ");
        uart_send_raw(HOST_UART, &message->s_nonce, sizeof(message->s_nonce));
        uart_send_raw(HOST_UART, &s_nonce, sizeof(s_nonce));
        #endif
       return false;
    }

    // Redundant(?) check to ensure there is a payload
    if(message->payload_size < 1) {
        #ifdef DEBUG
        debug_print("bad payload size: ");
        uart_send_raw(HOST_UART, &message->payload_size, sizeof(message->payload_size));
        #endif
        return false;
    }

    // Ensure that the recieved packet type is the expected type of packet to be recieved
    if(next_packet_type != message->msg_magic || next_packet_type == 0) {
        #ifdef DEBUG
        debug_print("bad packet type: ");
        uart_send_raw(HOST_UART, &message->msg_magic, sizeof(message->msg_magic));
        #endif
        return false;
    }

    // Ensure that the hmac of the packet is valid
    uint8_t hash[32];
    br_hmac_context ctx_hmac;
    br_hmac_init(&ctx_hmac, &ctx_hmac_key, sizeof(hash));
    br_hmac_update(&ctx_hmac, &message, sizeof(Message) - 36); //everything except hash (and frags)

    br_hmac_out(&ctx_hmac, hash);

    if(!memcmp(hash, message->payload_hash, sizeof(hash))) {
        #ifdef DEBUG
        debug_print("bad hash\n");
        uart_send_raw(HOST_UART, hash, sizeof(hash));
        uart_send_raw(HOST_UART, message->payload_hash, sizeof(message->payload_hash));
        #endif
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
    br_hmac_update(&ctx_hmac, message, sizeof(Message) - 36); //everything except hash (and frags)

    br_hmac_out(&ctx_hmac, message->payload_hash);
}

/**
 * Attempts to read a message packet from UART and parses it.
 * 
 * This is where the core Conversation logic occurs - the function
 * verifies that the incoming message is authenticated and valid and
 * parses the data accordingly.
 * 
 * It then attempts to craft a response to the incoming packet
 * 
 * If the incoming packet is invalid or times out, the internal state of the current conversation
 * is reset.
 * 
 * @return true if the message was valid and parseable
 */
bool parse_inc_message(void) {
    if(!uart_read_message(DEVICE_UART, &current_msg)) {
        //reset_state();
        return false;
    }

    //remove second verification if slow
    if(!verify_message(&current_msg) || !verify_message(&current_msg)) {
        safe_memset(&current_msg, 0, sizeof(Message));
        #ifdef DEBUG
        debug_print("msg verification fail\n");
        #endif
        reset_state();
        return false;
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
        #ifdef DEBUG
        debug_print("bad packet\n");
        #endif
        reset_state();
        return false;
    }

    #ifdef DEBUG
    debug_print("success!\n");
    #endif
    return true;
}

void send_next_message(void) {
    #ifdef DEBUG
    debug_print("attempting to send... ");
    #endif
    if(is_msg_ready) {
        #ifdef DEBUG
        debug_print("send!\n");
        #endif
        is_msg_ready = false;
        uart_send_message(DEVICE_UART, &current_msg);
    }
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
    send_next_message();
}

/**
 * @brief Creates a hello message as part of the Conversation protocol
 * 
 * This is the first packet to be sent in a sequence (Paired Fob -> Car)
 * 
 * See util.h for more information about the Conversation protocol.
 */
void gen_hello(void) {
    init_message(&current_msg);

    current_msg.msg_magic = HELLO;
    current_msg.target = TO_CAR;
    PacketHello* p = (PacketHello*) &current_msg.payload_buf;
    rand_get_bytes(challenge, sizeof(challenge));
    
    memcpy(&p->chall, &challenge, sizeof(challenge)); //is this safe?
    message_sign_payload(&current_msg, sizeof(PacketHello));
    next_packet_type = CHALL;
    is_msg_ready = true;
}

/**
 * @brief Creates a solution message as part of the Conversation protocol
 * 
 * This is the third packet to be sent in a sequence (Paired Fob -> Car)
 * 
 * This method depends on data from a hello packet and a challenge packet to
 * properly compute a valid response.
 * 
 * See util.h for more information about the Conversation protocol.
 */
void gen_solution(void) {

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
    br_sha256_update(&ctx_sha2, &c_nonce, sizeof(c_nonce));
    br_sha256_update(&ctx_sha2, &s_nonce, sizeof(s_nonce));
    br_sha256_out(&ctx_sha2, p->response);

    p->command_magic = UNLOCK_MGK;    

    CommandUnlock* c = &p->command;

    eeprom_read(c, sizeof(CommandUnlock), EEPROM_SIG_ADDR);
    message_sign_payload(&current_msg, sizeof(PacketSolution));

    next_packet_type = END;
    is_msg_ready = true;
}
/**
 * @brief Parses a recieved challenge message as part of the Conversation protocol
 * 
 * This method stores the challenge data from a given challenge message
 * 
 * See util.h for more information about the Conversation protocol.

 * @param message the message to be handled
 * 
 * @return true if the packet is valid.
 * @return false if the packet is invalid.
 */
bool handle_chall(Message* message) {
    if(message->payload_size != sizeof(PacketChallenge)) {
        return false;
    }
    s_nonce = message->s_nonce;
    PacketChallenge* p = (PacketChallenge*) &(message->payload_buf);

    memcpy(challenge_resp, p->chall, sizeof(challenge_resp));

    return true;
}

/**
 * @brief Parses a recieved end message as part of the Conversation protocol
 * 
 * As of right now, this method just forwards packet data to the host UART interface
 * 
 * See util.h for more information about the Conversation protocol.
 * 
 * @param message the message to be handled
 * 
 * @return true
 */
bool handle_end(Message* message) {

    uart_send_message(HOST_UART, message);
    reset_state();
    return true;
}

#endif


/************************************************************************************
 * The following section of functions is reserved only for use with a car fimrware. *
 * A fob (paired or unpaired) does not require these functions.                     *
 ************************************************************************************/

#ifdef CAR_TARGET

/**
 * @brief Parses a recieved hello message as part of the Conversation protocol
 * 
 * This method stores the challenge data from a given challenge message
 * 
 * See util.h for more information about the Conversation protocol.
 * 
 * @param message the message to be handled
 * 
 * @return true if the packet is valid.
 * @return false if the packet is invalid.
 */
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

/**
 * @brief Parses a recieved solution message as part of the Conversation protocol
 * 
 * This method verifies that the given solution to the previous challenges matches
 * the self-computed soluton.
 * 
 * See util.h for more information about the Conversation protocol.
 * 
 * @param message the message to be handled
 * 
 * @return true if the packet's solution is valid.
 * @return false if the packet is invalid or the solution is wrong.
 */
bool handle_solution(Message* message) {
    if(message->payload_size != sizeof(PacketSolution)) {
        return false;
    }
    PacketSolution* p = (PacketSolution*) &message->payload_buf;

    
    //verify the challenge/response is valid
    uint8_t auth_hash[32];

    br_sha256_context ctx_sha2;
    br_sha256_init(&ctx_sha2);
    br_sha256_update(&ctx_sha2, challenge, sizeof(challenge));
    br_sha256_update(&ctx_sha2, challenge_resp, sizeof(challenge_resp));
    br_sha256_update(&ctx_sha2, car_secret, sizeof(car_secret));
    br_sha256_update(&ctx_sha2, &c_nonce, sizeof(c_nonce));
    br_sha256_update(&ctx_sha2, &s_nonce, sizeof(s_nonce));
    br_sha256_out(&ctx_sha2, auth_hash);

    if(!memcmp(auth_hash, p->response, sizeof(auth_hash))) {
        return false;
    }
    
    //verify the features are all valid
    CommandUnlock* cmd = &p->command;

    if(cmd->feature_flags == 0) {
        verified_features = 0;
        return true;
    }

    if(cmd->feature_flags & 0x01) {
        if(cmd->feature_a.data[0] != CAR_ID) {
            return false;
        }
    }

    if(cmd->feature_flags & 0x02) {
        if(cmd->feature_b.data[0] != CAR_ID) {
            return false;
        }
    }

    if(cmd->feature_flags & 0x04) {
        if(cmd->feature_c.data[0] != CAR_ID) {
            return false;
        }
    }

    uint8_t feat_hash[32];
    br_sha256_context ctx_sha_f;
    br_sha256_init(&ctx_sha_f);
    br_sha256_update(&ctx_sha_f, &cmd->feature_flags, sizeof(cmd->feature_flags));

    br_sha256_update(&ctx_sha_f, &cmd->feature_a.data, sizeof(cmd->feature_a.data));
    br_sha256_update(&ctx_sha_f, &cmd->feature_a.signature, sizeof(cmd->feature_a.signature));

    br_sha256_update(&ctx_sha_f, &cmd->feature_b.data, sizeof(cmd->feature_b.data));
    br_sha256_update(&ctx_sha_f, &cmd->feature_a.signature, sizeof(cmd->feature_a.signature));
    
    br_sha256_update(&ctx_sha_f, &cmd->feature_c.data, sizeof(cmd->feature_c.data));
    br_sha256_update(&ctx_sha_f, &cmd->feature_a.signature, sizeof(cmd->feature_a.signature));

    br_sha256_out(&ctx_sha_f, feat_hash);

    br_ec_public_key pk;
    pk.curve = BR_EC_brainpoolP256r1;
    pk.q = factory_pub;
    pk.qlen = 65;
    uint32_t verification = br_ecdsa_i15_vrfy_raw(&br_ec_p256_m15, feat_hash, sizeof(feat_hash), &pk, &cmd->signature_multi, sizeof(cmd->signature_multi));
    
    if(!verification) {
        return false;
    }

    verified_features = cmd->feature_flags;

    return verification;
}


/**
 * @brief Creates a challenge message as part of the Conversation protocol
 * 
 * This is the second packet to be sent in a sequence (Car -> Paired Fob)
 * 
 * See util.h for more information about the Conversation protocol.
 */
void gen_chall(void) {

    init_message(&current_msg);

    current_msg.msg_magic = CHALL;
    current_msg.target = TO_P_FOB;

    PacketChallenge* p = (PacketChallenge*) &current_msg.payload_buf;
    rand_get_bytes(challenge, sizeof(challenge));
    memcpy(challenge, p->chall, sizeof(challenge));

    next_packet_type = SOLVE;
    is_msg_ready = true;
}


/**
 * @brief Creates an end message as part of the Conversation protocol
 * 
 * This is the fourth and final packet to be sent in a sequence (Car -> Paired Fob).
 * Currently, the only functionality of this packet is to send an unlock message (TODO)
 * 
 * See util.h for more information about the Conversation protocol.
 */
void gen_end(void) {

    init_message(&current_msg);

    current_msg.msg_magic = END;
    current_msg.target = TO_P_FOB;

    uint8_t unlock_msg[64];

    eeprom_read(unlock_msg, sizeof(unlock_msg), EEPROM_UNLOCK_ADDR);
    uart_send_raw(HOST_UART, unlock_msg, sizeof(unlock_msg));

    if(verified_features & 0x01) {
        eeprom_read(unlock_msg, sizeof(unlock_msg), EEPROM_FEAT_A_ADDR);
        uart_send_raw(HOST_UART, unlock_msg, sizeof(unlock_msg));
    }
    
    if(verified_features & 0x02) {
        eeprom_read(unlock_msg, sizeof(unlock_msg), EEPROM_FEAT_B_ADDR);
        uart_send_raw(HOST_UART, unlock_msg, sizeof(unlock_msg));
    }

    if(verified_features & 0x04) {
        eeprom_read(unlock_msg, sizeof(unlock_msg), EEPROM_FEAT_B_ADDR);
        uart_send_raw(HOST_UART, unlock_msg, sizeof(unlock_msg));
    }

    next_packet_type = HELLO;
    verified_features = 0;
    is_msg_ready = true;
}
#endif

/**
 * @brief Resets the internal state of the Conversation sequence.
 * 
 * Creates new nonces.
 *
 * The next expected packet type is set to HELLO (although this will only be used with the Car)
 * 
 * The stored challenge and challenge responses are reset.
 */
void reset_state(void) {

    rand_get_bytes(&c_nonce, sizeof(c_nonce));
    rand_get_bytes(&s_nonce, sizeof(s_nonce));

    next_packet_type = HELLO;
    is_msg_ready = false;

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
    uint8_t e_factory[32] = FACTORY_ENTROPY;
    br_hmac_drbg_init(&ctx_rand, &br_sha256_vtable, e_factory, sizeof(e_factory));

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


/**
 * @brief gets random bytes from internal prng
 * 
 * @param out buffer to store random bytes
 * @param len number of bytes to write
 */
void rand_get_bytes(void* out, size_t len) {
    br_hmac_drbg_generate(&ctx_rand, out, len);
}