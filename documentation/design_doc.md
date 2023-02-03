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
- Features packaged for one car should not be enableable on another (SR6)
    - Signing of features will include the car it was intented for.

---

## Devices

There are two types of devices - cars and fobs. They all share common variables used for integrity verification in handshakes:
- `priv_key`: device unique private key
- `pub_key`: device unique pub key
- `signature`: factory signature of public key

These are all generated in factory and are unique to each device (except for factory signature) - they *never* change.

---

## Messages

Messages are packets sent over UART1 from one embedded device to another. While all devices can send messages, only fobs can initiate a conversation between the two devices. They have the following structure:

```c
struct Message {
    uint8_t magic; //packet type
    uint16_t nonce; //counter for current message. Only packets with a higher nonce than the last will be considered
    uint8_t[64] payload; //type-specific payload
    uint8_t[64] signature; //signature over payload
}
```

The goal of the `Message` data structure is to prevent reply attacks and ensure the integrity of each message. 

The following are valid packet types (magics):
```
key_exchange (0x59) | any <--> any
unlock       (0x6A) | paired fob <--> car
pair         (0x7B) | unpaired fob <--> paired fob
```

If a packet has either an invalid magic or nonce, or is sent to the wrong device type, it will be discarded. 

`hello` messages can reset a nonce. (?)

Messages are signed with either device public keys or factory public keys. If the signature does not match the contents, it will be discarded.

    The `key_exchange` message is special in that it is the only message that does not require a valid device signature. Instead, the payload contents (a public key) **must** be signed using the factory's private key. This is to ensure only legitimate devices can communicate with one another.

---

## Conversations

Conversations are a **two way** handshake between embedded devices comprised of *messages*. The implementation goal is to ensure communication secrecy and disallow replay attacks (As per SR3). 

Before any conversation, it is necessary to exchange certificates to ensure messages have not been tampered with using the `key_exchange` message. If a **factory-signed** public key is presented, it is stored in the recieving device and used to verify succeeding messages in the sequence.

<img src=../assets/img/2023-02-03-14-23-40.png>

    coloring is hard - the first two messages are part of the key exchange; the following four are part of the conversation.

Each conversation consists of a **client** and a **server**. The **client** always initiates the conversation with a `hello` message and the following `payload`:

```c
struct hello {
    uint8_t[64] fob_entropy; //64 bytes of random data to be used in a handshake
}
```

**Only fobs can initialize a conversation (send a hello message).**

Upon recieving a valid hello message, a conversation is initiated between the sender and recipient of the messages, comprised of a two way *Challenge* followed by an accept/reject message by the server.

The next step is an `acknowledge` message:

```c
struct ack {
    uint8_t[64] chal; //f(c_entropy, c_pub, s_entropy)
}
```

Upon verification that the response is legit, the client will send a request packet to actually execute a command:

```c
struct exe {
    uint8_t[32] resp; //f(chal)
    uint8_t[32] command; //command specific data
}
```

Finally, an accept/reject message is sent to the client from the server:

```c
struct status {
    uint8_t[64] msg; //char[] message
}
```

<img src=../assets/img/2023-02-03-14-44-16.png>

---

## Concerns

## Entropy

Entropy is hard. By using entropy from two completely separate devices, we can hopefully make it much harder to create predictable / repeatable patterns.

We plan on having a seed, generated by a combination of the starting temperature, the RAM of the device on startup (which appears to be semi-random according to this [study](https://eprint.iacr.org/2013/304.pdf)). Our rand() function will be a combination of the seed and the analog system of the two devices which have a conversation. 

Our hope is that even if attackers can reliably determine the randomness of one device, the second device can add a layer of additional randomness.

## Pin Concerns

A six digit pin is easily brute-forcible. Normally, this could be prevented using a brute-force detection system, but by completely reflashing the board, this can be bypassed.



---