import base64
from Crypto.Hash import SHA256
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS

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

    def verify_feature(self, feature):
        feature = base64.b64decode(feature)
        #do stuff here

    def execute(self, cmd):
        if cmd == 'unlock':
            if self.car_key == 0:
                return 'error: not paired'
        elif cmd == 'feature_add':
            return self.verify_feature(cmd.split(' ')[1])

class Car:
    car_key = []
