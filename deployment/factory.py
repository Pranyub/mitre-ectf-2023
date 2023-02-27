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
    features = {}
    empty = {'data': b'\x00' * 32}
    for x in range(1, 4):
        feature = {}

        feature['data'] = (
        car_id.to_bytes(1, 'little')
        + x.to_bytes(1, 'little')
        + get_random_bytes(30)
        )

        m = hashlib.sha256()
        m.update(feature['data'])
        feature['sig'] = signer.sign(m)
        features[str(x)] = feature




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
    car['key'] = get_random_bytes(16).hex()

    car_secrets['car' + str(x)] = get_random_bytes(16).hex()

f.write(json.dumps(car_secrets))