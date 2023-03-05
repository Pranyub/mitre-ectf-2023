import serial

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

#query = b'\x5e'