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
    - Salted SHA-2 Hash of car pin

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
- 6 digits
- needs some type of brute force prevention
    - 1s time limit (except in cases of brute-force detection)
    - In cases where brute force is detected (many packets sent over very short time), begin expontentially growing rate at which packets are processed

## Side-Channel Considerations:
- AES/ChaCha20/RSA -> Key leaks
- Pin hash -> Hash leaks
- Strcmps -> Timing leaks
- Instruction skipping countermeasures

---

## Messages

Messages are packets sent over UART1 from one embedded device to another. While all devices can send messages, only fobs can initiate a conversation between the two devices. They have the following structure:

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
```rust
unlock (0x33) | paired fob -> car
pair   (0x44) | unpaired fob -> paired fob
```

If a packet has either an invalid magic or nonce, or is sent to the wrong device type, it will be discarded.

---

## Challenges

A challenge is a special type of transaction that is used to verify the authenticity of the devices in a conversation. The general structure of the message is listed below:

```c
struct Challenge{
    uint8_t magic; //packet type
    uint16_t nonce; //nonce
    uint8_t[16] challenge; //A specific randomly generated 16-byte pin used for the challenge.
    uint8_t[16] hash; // The challenge hashed by a custom hashing algorithm - prevents forging challenge messages
    uint8_t[64] signature; //Signature, this can be hashed in fob to further verify the original sender
    uint8_t[25] padding; //random bit padding to reach 128 bytes
}
```

The challenge will be a randomly generated one time string that will be encrypted in __ECDSA__. Upon recieving a solution, the original challenge is verified. There are two types of challenge solutions, happening when a paired fob sends a challenge during pairing and when car sends a challenge to a paired fob during an unlock request.

We expect our challenge mechanic will have the most attacks targeted towards it, as it is integral to the functionality of our fob devices.

---

## Conversations

Conversations are a **two way** handshake between embedded devices comprised of *messages*. The implementation goal is to ensure communication secrecy and disallow replay attacks (As per SR3). 

Each conversation consists of a **client** and a **server**. The **client** always initiates the conversation with a `hello` message, as shown below:

```python
|MAGIC |   BEGIN CONVERSATION   |   NO SIG    |   PADDING   |
|[0x33]|['hello'\x00\x00...\x00]|[\x00...\x00]|[\x00...\x00]|
```
As mentioned above, only fobs can initialize a conversation (send a hello message).

Upon recieving a valid hello message, a conversation is initiated between the sender and recipient of the messages, comprised of packets specified below.

---

## Unlock Request

An unlock request is a type of conversation that is initiated by a paired fob to the car it is paired with. These messages have the default structure as shown below:

```c
struct Unlock_packet {
    uint8_t magic;
    uint8_t car_id;
    char[64] feature_list; //some kind of encoding. longer = better? (more entropy for rsa)
    char[64] rand_bytes; //rand
    uint8_t[318] pad; //random bit padding to get 512 bytes.
    uint8_t[32] hash; //hash over above?
    uint8_t[32] signature; //signature over hash (or maybe just hash contents instead)
}
```
Upon recieving an unlock request from the fob, the car will first check if there is are any features needed to be added. If there are, then the car will verify that the features are intended for it and then add the features to the car. 
Afterwards, the car will send a challenge message (see above) to the fob.

A paired fob, which contains the car's encryption key, will be able to encrypt the challenge and send it back to the car. At this point, the car will be able to use its private key to decrypt and verify that the challenge sent and recieved match (SR1). Upon verification of the challenge's correctness, it will send it's unlock request.

---

## Pair Request

A pair request is a type of conversation that is initiated by an unpaired fob to a fob that it is paired with. These messages have the default structure as shown below:

```c
struct Pair_packet{
    uint8_t magic;
    char[64] rand_bytes; //rand 
    uint8_t[32] hash; //hash
    uint8_t[32] signature; //signature over hash (or maybe just hash contents instead)
}
```

As an unpaired fob does not have much ... ???? [dunno how this works]