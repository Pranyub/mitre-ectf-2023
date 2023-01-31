# Embedded System Design Document

## Paired Fob
- Requirements:
    - Must open paired car
    - Must be able to send car secrets to unpaired fob
    - Must accept and verify car packaged features
- Contains:
    - Universal fob key
    - Car encryption key
    - Car id
    - (Salted?) SHA-2 Hash of car pin

## Unpaired Fob
- Requirements:
    - Must be able to turn into paired key with pin
- Contains:
    - Universal fob key

## Car
- Requirements:
    - Must unlock when given valid signal
    - Must be firmware-upgradable
- Contains:
    - Vendor public key (firmware verification)
    - Car encryption key
    - Car id

---

## Packaged Feature
- Requirements:
    - Must not be forgable
    - Must be unique to each car
- Contains:
    - Feature (text)
    - Vendor signature
    - Car id

## Car Feature
- Requirement: Unique to each car
    - Contains car secret, approved feature
- Requirement: Non-forgeable
    - Signed with vendor private key


## Pairing Pin
- 6 digits? (double check)
- needs some type of brute force prevention
    - 1s time limit (except in cases of brute-force detection)

## Side-Channel Considerations:
- AES/ChaCha20/RSA -> Key leaks
- Pin hash -> Hash leaks
- Strcmps -> Timing leaks
- Instruction skipping countermeasures

---

## Messages

Messages are packets sent over UART1 from one embedded device to another. Only fobs can initialize a conversation. They have the following structure:

```c
struct Message {
    uint8_t magic; //packet type
    uint16_t nonce; //counter for current message. Only packets with a higher nonce than the last will be considered
    uint8_t[64] payload; //type-specific payload
    uint8_t[64] signature; //optional signature
    uint8_t[14] padding; //0x00 padding to reach 144 bytes
}
```

The following are valid packet types:
```
unlock (0x33) | paired fob -> car
pair   (0x44) | unpaired fob -> paired fob
```

If a packet has either an invalid magic or nonce, or is sent to the wrong device type, it will be discarded.

---

## Conversations

Conversations are a **two way** handshake between embedded devices comprised of *messages*. The implementation goal is to ensure communication secrecy and disallow replay attacks.

Each conversation consists of a **client** and a **server**. The **client** always initiates the conversation with a `hello` message.

e.g.
```python
|MAGIC |   BEGIN CONVERSATION   |   NO SIG    |   PADDING   |
|[0x33]|['hello'\x00\x00...\x00]|[\x00...\x00]|[\x00...\x00]|
```

---

## Unlock Request
```c
struct unlock_packet {
    uint8_t magic;
    uint8_t car_id;
    char[64] feature_list; //some kind of encoding. longer = better? (more entropy for rsa)
    char[64] rand_bytes; //rand
    uint8_t[383] pad; //padding to get 512 bytes (necesssary?)
    uint8_t[32] hash; //hash over above?
    uint8_t[32] signature; //signature over hash (or maybe just hash contents instead)
}
```