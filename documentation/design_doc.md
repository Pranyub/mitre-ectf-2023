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

