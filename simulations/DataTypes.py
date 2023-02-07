import json
import random
from Crypto.Hash import SHA256
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS

def byte_xor(ba1, ba2):
    return bytes([_a ^ _b for _a, _b in zip(ba1, ba2)])


class Message:
    def __init__(self, magic='0', nonce_c=b'\x00'*8, nonce_s=b'\x00'*8, size=0, payload=[], sig=0):
        self.magic = magic
        self.nonce_c = nonce_c
        self.nonce_s = nonce_s
        self.size = size
        self.payload = payload
        self.sig = sig

    def jsonify(self):
        return {
            'magic': self.magic,
            'nonce_c': self.nonce_c.hex(),
            'nonce_s': self.nonce_s.hex(),
            'size': self.size,
            'payload': self.payload.hex(),
            'sig': self.sig.hex()
        }
    
    def unjsonify(jsond):
        return Message(
            jsond['magic'],
            bytes.fromhex(jsond['nonce_c']),
            bytes.fromhex(jsond['nonce_s']),
            jsond['nonce_c'],
            bytes.fromhex(jsond['payload']),
            bytes.fromhex(jsond['sig'])
        )

# Turns out each device pretty much does the same thing, except for the stuff in the command, so abstraction ftw!!11!
# Let me have this before the C rewrite lol
class Device:
    def __init__(self, priv, pub, c_id, c_secret, sig, f_pub):
        
        #device specific pub/priv key
        self.priv = priv
        self.pub = pub

        #car id (0 for unpaired fob)
        self.c_id = c_id

        #factory signature of device pub key
        self.sig = sig

        #factory public key
        self.factory_pub = f_pub

        #secret value used in challenge (technically doesnt have to be secret?)
        self.secret = 0

        #shared car secret (static)
        self.c_secret = c_secret

        #==== Message Specific Stuff ====

        #public key of communicating device
        self.client_pub = [b'\x00'] * 64
        
        #nonce of communicating device
        self.client_nonce = b'\x00' * 8

        #nonce of this device
        self.server_nonce = b'\x00' * 8
        #================================
        
        #init rand (for now just use builtin)
        # self.rand = random.seed()
    
    # It's pseudorandom, but im not sure thats gonna be an issue at all
    def rand(self, num_bytes):
        return random.randbytes(num_bytes)

    #==== Conversation Functions ====

    def handle_key_exchange(self, message: Message):
        
        #Python messes up payload lens
        #if message.size != 64 or len(message.payload) != 64:
        #    return {'error': 'Invalid Payload Length'}
        
        # Check if signature is valid
        verifier = DSS.new(self.factory_pub, 'fips-186-3')
        try:
            verifier.verify(SHA256.new(message.payload), message.sig)
        except:
            return {'error': 'Bad Signature'}
        
        # Set the active public key
        self.client_pub = message.payload
        
        # Technically, this returns some magic but whatever
        return {'message': b'Success!'} #...although 'Success!' can be a magic ;)

    def handle_start(self, message: Message):
        self.secret = self.rand(32)
        return {'message': self.secret}
    
    def handle_chall(self, message: Message):
        #if message.size != 32 or len(message.payload) != 32:
        #    return {'error': 'Invlaid Payload Length'}
        h = SHA256.new(message.payload)
        h.update(self.client_pub)
        self.secret = self.rand(32)

        # I'll be honest i have no idea that this is for... just use aes or smth
        return {'message': b'\x99' + byte_xor(self.c_secret, h.digest())}
    
    def handle_resp(self, message: Message):
        #if(message.size != 32 or len(message.payload) != 32):
        #    return {'error': 'Invlaid Payload Length'}
        h = SHA256.new(self.secret)
        h.update(self.pub.export_key(format='raw'))

         #maybe use AES instead of xor?
        return {'message': byte_xor(self.c_secret, h.digest())}

    # reset variables
    def cleanup(self):
        self.client_pub = [0] * 64
        self.secret = self.rand(32) #not necessary
        self.client_nonce = self.rand(8)
        self.server_nonce = self.rand(8) #maybe necessary?


    def wrap(self, magic, payload: dict):
        message = Message(magic=magic, nonce_s=self.server_nonce, nonce_c=self.client_nonce)
        if 'error' in payload.keys():
            message.payload = ('error: ' + payload['error']).encode()
        else:
            message.payload = payload['message']
        message.size = len(message.payload)
        h = SHA256.new(message.magic.encode())
        h.update(message.nonce_c)
        h.update(message.nonce_s)
        print(message.payload)
        if(type(message.payload) == str):
            h.update(message.payload.encode())
        else:
            h.update(message.payload)
        signer = DSS.new(self.priv, 'fips-186-3')
        message.sig = signer.sign(h)
        return message

    # message handler
    def on_message(self, message: Message):
        if message.magic == 'key_exchange_a':
            self.server_nonce = self.rand(8)
            self.client_nonce = message.nonce_c
            return self.wrap('key_exchange_b', self.handle_key_exchange(message))
        
        elif message.magic == 'key_exchange_b':
            self.server_nonce = message.nonce_s
            return self.wrap('start', self.handle_start(message))

        if message.nonce_s != self.server_nonce or message.nonce_c != self.client_nonce:
            return self.wrap('err', {'error': 'invalid nonce'})
        
        elif message.magic == 'start':
            return self.wrap('chall', self.handle_chall(message))

        elif message.magic == 'chall':
            return self.wrap('cmd', self.handle_resp(message))
        
        elif message.magic == 'cmd':
            return self.wrap('fin', self.handle_cmd(message))
        elif message.magic == 'fin':
            self.cleanup()
        return self.wrap({'error': 'invalid magic'})



    # ===============================

class Fob(Device):

    def verify_and_add(self, feature):
        feature = json.loads(feature)

        if feature.car_id != self.car_id:
            return 'error: wrong car'

        elif self.features.count(feature) > 0:
            return 'error: feature already exists'

        h = SHA256.new(feature.f)
        verifier = DSS.new(self.f_pub, 'fips-186-3')
        try:
            verifier.verify(h, feature.s)
        except:
            return 'Error: Bad Signature'
        
        self.features.append(feature)
        return 'success: added feature'

    def unlock(self):
        self.client_nonce = self.rand(8)
        msg = self.wrap('key_exchange_a', {'message': self.pub.export_key(format='raw')})
        msg.sig = self.sig
        return msg
    
    def __repr__(self):
        return f"fob with id {self.c_id}"

class Car(Device):
    def __repr__(self):
        return f"car with id {self.c_id}"
    
    def handle_resp(self, message: Message):
        if message.payload[:1] != b'\x99':
            return {'error':'bad command'}
        message.payload = message.payload[1:]

    
    pass


class Factory:
    def __init__(self):
        self.priv = ECC.generate(curve='P-256')
        self.pub = self.priv.public_key()

    def gen_feature(self, feature, car):
        export = {'feature': feature, 'car_id': car}
        export = json.dumps(export).encode()
        h = SHA256.new(export)
        signer = DSS.new(self.priv, 'fips-186-3')
        signature = signer.sign(h)
        return json.dumps({'f': export, 's': signature})
    
    def gen_car_fob(self, c_id):
        c_priv = ECC.generate(curve='P-256')
        c_pub = c_priv.public_key()

        f_priv = ECC.generate(curve='P-256')
        f_pub = f_priv.public_key()

        signer = DSS.new(self.priv, 'fips-186-3')

        c_secret = random.randbytes(32)

        car = Car(c_priv, c_pub, c_id, c_secret, signer.sign(SHA256.new(c_pub.export_key(format='raw'))), self.pub)

        signer = DSS.new(self.priv, 'fips-186-3')
        fob = Fob(f_priv, f_pub, c_id, c_secret, signer.sign(SHA256.new(f_pub.export_key(format='raw'))), self.pub)

        return (car, fob)
    
    def gen_unpaired_fob(self):
        f_priv = ECC.generate(curve='P-256')
        f_pub = f_priv.public_key()
        signer = DSS.new(self.priv, 'fips-186-3')
        fob = Fob(f_priv, f_pub, 0, [0] * 32, signer.sign(SHA256.new(f_pub.export_key(format='raw'))), self.pub)

        return fob