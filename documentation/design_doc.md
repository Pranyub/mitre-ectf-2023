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
    - (Salted) SHA-2 Hash of car pin

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
    - Signature w/ vendor key
    - Car id
