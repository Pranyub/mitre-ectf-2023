import serial
import hmac
import hashlib
import struct

dev = '/dev/cu.usbmodem0E2349B71'

ser = serial.Serial(dev, 115200, timeout=1)

FACTORY_ENTROPY = bytes.fromhex('df013746886b5dcc')
PAIR_SECRET  = bytes.fromhex('41414141414141414141414141414141')

nonce_c = b''
nonce_s = bytes.fromhex('deadbeefdeadbeef')
chall = bytes.fromhex('abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789')
msg = {}
a = b'abcd'
def parse_msg(msg_raw, m=msg):
    global nonce_c
    magic = msg_raw[:4].decode()
    if magic != '0ops':
        print("BAD MAGIC")
    msg_raw = msg_raw[4:]
    m['target'] = bytes(msg_raw[0])
    m['magic'] = msg_raw[1:8]
    m['nonce_c'] = msg_raw[8:16]
    nonce_c = m['nonce_c']
    m['nonce_s'] = msg_raw[16:24]
    m['payload_size'] = msg_raw[24:28]
    m['payload_buf'] = msg_raw[28:540]
    m['payload_hash'] = msg_raw[540:572]
    m['padding'] = msg_raw[572:]

    h = hmac.new(PAIR_SECRET, msg_raw[:(len(msg_raw) - 36)], hashlib.sha256)

    print(h.hexdigest())

def make_chall():
    out = b''
    out += b'\x70' #paired fob target
    out += b'\x43\x00\x00\x00\x00\x00\x00' #challenge magic
    out += nonce_c
    out += nonce_s
    out += struct.pack('<I', 32)
    out += chall + b'\x00' * (512 - 32)

    h = hmac.new(PAIR_SECRET, out[:(len(out) - 36)], hashlib.sha256)
    out += h.digest()
    out += b'\x00' * 4
    return b'0ops' + out

def sign_msg(m=msg):
    h = hmac.new(PAIR_SECRET, m['target'], hashlib.sha256)
    h.update(m['magic'])
    pass