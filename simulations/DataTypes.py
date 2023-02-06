import json
import random
from Crypto.Hash import SHA256
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS

def byte_xor(ba1, ba2):
    return bytes([_a ^ _b for _a, _b in zip(ba1, ba2)])


class Message:
    def __init__(self, magic='0', nonce_c=0, nonce_s=0, size=0, payload=[], sig=0):
        self.magic = magic
        self.nonce_c = nonce_c
        self.nonce_s = nonce_s
        self.size = size
        self.payload = payload
        self.sig = sig
        self.error = None # debug only


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
        self.client_pub = [0] * 64
        
        #nonce of communicating device
        self.client_nonce = 0

        #nonce of this device
        self.self_nonce = 0
        #================================
        
        #init rand (for now just use builtin)
        # self.rand = random.seed()
    
    # It's pseudorandom, but im not sure thats gonna be an issue at all
    def rand(num_bytes):
        return random.randbytes(num_bytes)

    #==== Conversation Functions ====

    def handle_key_exchange(self, message: Message):
        if message.size != 64 or len(message.payload) != 64:
            return 'Error: Invalid Payload Length'
        
        # Check if signature is valid
        verifier = DSS.new(SHA256.new(self.factory_pub), 'fips-186-3')
        try:
            verifier.verify(message.payload, message.sig)
        except:
            return 'Error: Bad Signature'
        
        # Set the active public key
        self.client_pub = message.payload

        # Set nonces
        self.client_nonce = message.nonce_c
        self.self_nonce = int.from_bytes(self.rand(8), 'big')
        
        # Technically, this returns some magic but whatever
        return {'message': b'Success!'} #...although 'Success!' can be a magic ;)

    def handle_start(self, message: Message):
        self.secret = self.rand(32)
        return self.secret
    
    def handle_chall(self, message: Message):
        if message.size != 32 or len(message.payload) != 32:
            return {'Error': 'Invlaid Payload Length'}
        h = SHA256.new(message.payload)
        h.update(self.client_pub)
        self.secret = self.rand(32)

       
        return {'message': byte_xor(self.c_secret, byte_xor(h.digest(), self.secret))}
    
    def handle_resp(self, message: Message):
        if(message.size != 32 or len(message.payload) != 32):
            return {'error': 'Invlaid Payload Length'}
        h = SHA256.new(self.secret)
        h.update(self.pub)

         #maybe use AES instead of xor?
        return {'message': byte_xor(self.c_secret, byte_xor(h, self.secret))}

    # reset variables
    def cleanup(self):
        self.client_pub = [0] * 64
        self.secret = self.rand(32) #not necessary
        self.client_nonce = 0
        self.self_nonce = int.from_bytes(self.rand(8), 'big') #maybe necessary?


    def wrap(self, magic, payload):
        message = Message()

    # message handler
    def on_message(self, message: Message):
        if message.magic == 'key_exchange_a':
            return self.wrap('key_exchange_b', self.handle_key_exchange(message))
        
        if message.nonce_c != self.client_nonce:
            return self.wrap({'error': 'invalid nonce'})
        
        if message.magic == 'key_exchange_b':
            return self.wrap('start', self.handle_start)
        
        elif message.magic == 'start':
            return self.wrap('chall', self.handle_chall)

        elif message.magic == 'chall':
            return self.wrap(self.handle_resp)
        
        elif message.magic == 'cmd':
            return self.wrap('cmd', self.handle_cmd)
        
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

        pass
    
    def __repr__(self):
        return f"fob with id {self.c_id}"

class Car(Device):
    def __repr__(self):
        return f"car with id {self.c_id}"
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
        fob = Fob(f_priv, f_pub, c_id, c_secret, signer.sign(SHA256.new(f_pub.export_key(format='raw'))), self.pub)

        return (car, fob)
    
    def gen_unpaired_fob(self):
        f_priv = ECC.generate(curve='P-256')
        f_pub = f_priv.public_key()
        signer = DSS.new(self.priv, 'fips-186-3')
        fob = Fob(f_priv, f_pub, 0, [0] * 32, signer.sign(SHA256.new(f_pub.export_key(format='raw'))), self.pub)

        return fob