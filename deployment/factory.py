#!/bin/python3
from Crypto.Hash import SHA256
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS
from Crypto.Random import get_random_bytes
import hashlib
import json
import sys

key = ECC.generate(curve='P-256')
signer = DSS.new(key, 'fips-186-3')

def gen_features(car_id):
    features = []

    for x in range(1, 4):
        feature = {}

        feature['data'] = (
        car_id.to_bytes(1, 'little')
        + x.to_bytes(1, 'little')
        + get_random_bytes(30)
        ).hex()

        m = SHA256.new(bytes.fromhex(feature['data']))
        feature['sig'] = signer.sign(m).hex()
        features.append(feature)

    return features




if(len(sys.argv) != 2):
    print("bad args")
    exit()

secrets_dir = sys.argv[1]

f = open(secrets_dir + '/car_secrets.json', 'w')

car_secrets = {}

car_secrets['pubkey'] = key.public_key().export_key(format='raw').hex()
car_secrets['privkey'] = key.export_key(format='DER').hex()

for x in range(255):
    car = {}
    car['shared_key'] = get_random_bytes(32).hex()
    car['feature_seed'] = get_random_bytes(32).hex()
    car['features'] = gen_features(x)
    car_secrets['car' + str(x)] = car

f.write(json.dumps(car_secrets))

