# Proposed System Design Document (v2)

## Security Requirements (For Reference)

These security requirements **must** be met:
- SR1: A car should only unlock and start when the user has an authentic fob that is paired with the car
- SR2: Revoking an attacker’s physical access to a fob should also revoke their ability to unlock the associated car
- SR3: Observing the communications between a fob and a car while unlocking should not allow an attacker to unlock the car in the future
- SR4: Having an unpaired fob should not allow an attacker to unlock a car without a corresponding paired fob and pairing PIN
- SR5: A car owner should not be able to add new features to a fob that did not get packaged by the manufacturer
- SR6: Access to a feature packaged for one car should not allow an attacker to enable the same feature on another car


## Design Constraints

There are a few requirements within the system design that need to be met in order to achieve all Security Requirements:
- The car **must** be able to verify the paired fob's integrity (SR1)
    - A fob can sign a message using a car's private key
- Replay attacks **must** be unachievable (SR2)
    - A random nonce can be used in communication
- Communication is meaningless to those without private keys (SR3)
    - Communication can be encrypted using ECC
- The pairing pin is **not** brute-forceable (SR4)
    - There is a timeout on pairs permitted; some sort of reflash protection is needed as well
- Features are not forgeable (SR5)
    - Features are **signed** by the manufacturer
- Features are unique to each car (SR6)



---

## Messages

Messages are packets sent over UART1 from one embedded device to another. While all devices can send messages, only fobs can initiate a conversation between the two devices. They have the following structure:

```c
struct Message {
    uint8_t magic; //packet type
    uint16_t nonce; //counter for current message. Only packets with a higher nonce than the last will be considered
    uint8_t[64] payload; //type-specific payload
    uint8_t[64] signature; //(optional) signature
    uint8_t[14] padding; //0x00 padding to reach 144 bytes
}
```

The following are valid packet types:
```
unlock (0x33) | paired fob -> car
pair   (0x44) | unpaired fob -> paired fob
```

If a packet has either an invalid magic or nonce, or is sent to the wrong device type, it will be discarded.

The goal of the `Message` data structure is to prevent reply attacks and ensure the integrity of each message. 

---

## Conversations

Conversations are a **two way** handshake between embedded devices comprised of *messages*. The implementation goal is to ensure communication secrecy and disallow replay attacks (As per SR3). 

Each conversation consists of a **client** and a **server**. The **client** always initiates the conversation with a `hello` message, as shown below:

```python
|MAGIC |   BEGIN CONVERSATION   |   NO SIG    |   PADDING   |
|[0x33]|['hello'\x00\x00...\x00]|[\x00...\x00]|[\x00...\x00]|
```
**Only fobs can initialize a conversation (send a hello message).**

Upon recieving a valid hello message, a conversation is initiated between the sender and recipient of the messages, comprised of a two way *Challenge* followed by an accept/reject message by the server.

---

## Challenges

A challenge is a special type of transaction that is used to verify the authenticity of the devices in a conversation. The general structure of the message is listed below:

```c
struct Challenge {
    uint8_t[16] challenge; //A specific randomly generated 16-byte pin used for the challenge.
    uint8_t[16] hash; // The challenge hashed by a custom hashing algorithm - prevents forging challenge messages
    uint8_t[64] signature; //Signature, this can be hashed in fob to further verify the original sender
    uint8_t[25] padding; //random bit padding to reach 128 bytes - provides minimal obfuscation
}
```

The challenge will be a randomly generated one time string that will be encrypted in __ECDSA__. Upon recieving a solution, the original challenge is verified. There are two types of challenge solutions, happening when a paired fob sends a challenge during pairing and when car sends a challenge to a paired fob during an unlock request.

We expect our challenge mechanic will have the most attacks targeted towards it, as it is integral to the functionality of our fob devices.

---

## Unlock Request

An unlock request is a type of conversation that is initiated by a paired fob to the car it is paired with. These messages have the default structure as shown below:

```c
struct Unlock_packet {
    uint8_t magic;
    uint8_t car_id;
    char[64] feature_list; //some kind of encoding. longer = better? (more entropy for rsa)
    char[64] rand_bytes; //rand
    uint8_t[383] pad; //random bit padding to get 512 bytes
    uint8_t[32] hash; //hash over above?
    uint8_t[32] signature; //signature over hash (or maybe just hash contents instead)
}
```
Upon recieving an unlock request from the fob, the car will first check if there is are any features needed to be added. If there are, then the car will verify that the features are intended for it and then add the features to the car. 
Afterwards, the car will send a challenge message (see above) to the fob.

A paired fob, which contains the car's encryption key, will be able to encrypt the challenge and send it back to the car. At this point, the car will be able to use its private key to decrypt and verify that the challenge sent and recieved match. Upon verification of the challenge's correctness, it will send it's unlock request.

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