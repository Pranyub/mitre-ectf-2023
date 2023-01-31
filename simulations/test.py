import base64
import json
from Crypto.Hash import SHA256
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS

class Factory:
    def __init__(self):
        self.priv = ECC.generate(curve='P-256')
        self.pub = self.priv.public_key()

    def generate_feature(self, feature, car):
        export = {'feature': feature, 'car_id': car}
        export = json.dumps(export).encode()
        h = SHA256.new(export)
        signer = DSS.new(self.priv, 'fips-186-3')
        signature = signer.sign(h)
        return json.dumps({'f': export, 's': signature})


class Fob:
    def __init__(self, signature, fob_key, car=None):
        if car:
            self.car_key = car.key
            self.car_id = car.id
            self._paired = True
        else:
            self.car_key = 0
            self.car_id = 0
            self._paired = False

        self.signature = signature
        self.fob_key = fob_key
        self.features = []

    def verify_and_add(self, feature):
        feature = json.loads(feature)

        if feature.car_id != self.car_id:
            return 'error: wrong car'

        elif self.features.count(feature) > 0:
            return 'error: feature already exists'

        h = SHA256.new(feature.f)
        verifier = DSS.new(self.signature, 'fips-186-3')
        try:
            verifier.verify(h, feature.s)
        except:
            return 'error: bad signature'
        
        self.features.append(feature)
        return 'success: added feature'

    def execute(self, cmd):
        if cmd == 'unlock':
            if self.car_key == 0:
                return 'error: not paired'
            
        elif cmd == 'feature_add':
            return self.verify_and_add(cmd.split(' ')[1])
        
        return 'error: invalid command'

class Car:
    car_key = []
