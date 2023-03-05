import serial
import struct

car = serial.Serial('/dev/cu.usbmodem0E2349AA1', 115200, timeout=1)
fob = serial.Serial('/dev/cu.usbmodem0E2349B71', 115200, timeout=1)

print(fob.read(1000))
print(car.read(1000))


def read():
    f = fob.read(10000)
    c = car.read(10000)
    return (f, c)

def get_pkts(f):
    out = []
    while f.find(b'Recieved Message: ') != -1:
        f = f[f.index(b'Recieved Message: ') + 19:]
        out.append(f[:472])
    return out

def get_pkts_hex(f):
    out = get_pkts(f)
    return [x.hex() for x in out]

def get_pkts_parsed(f):
    packets = get_pkts(f)

    out = []

    for p in packets:
        out.append({
            'target': chr(p[0]),
            'magic': chr(p[1]),
            'c_nonce': p[8:16].hex(),
            's_nonce': p[16:24].hex(),
            'payload_size': struct.unpack('<I', p[24:28]),
            'payload': p[28:436].hex(),
            'hash': p[436:436+32].hex()
        })
    return out

import json

def get_sent_pkts(f):
    out = []
    while f.find(b'send!\n0ops') != -1:
        f = f[f.index(b'Recieved Message: ') + 12:]
        out.append(f[:472])
    return out

def get_pkg():
    f = open('packageout/pak1', 'r').read()
    m = json.loads(f)
    return m

m = get_pkg()

feat = (b'\x1a' + bytes.fromhex(m['feature'])).ljust(128)
sig = (b'\x2b' + bytes.fromhex(m['sigs']['1'])).ljust(128)


def capt_packets():
    packets = []
    while(len(packets) < 10):
        try:
            f = fob.read(10000)
            c = car.read(10000)

            if(len(f) > 0 and len(c) > 0):
                print('got packets')

                f_pkts = get_pkts_parsed(f)
                c_pkts = get_pkts_parsed(c)

                unlock_msg = c[c.index(b'Car Unlocked'):c.index(b'Car Unlocked') + 64].decode()

                sequence = [x for y in zip(f_pkts, c_pkts) for x in y]

                sequence.append(unlock_msg)

                packets.append(sequence)
        except KeyboardInterrupt:
            break

    f = open('packets.txt', 'w')
    f.write(json.dumps(packets))


#query = b'\x5e'