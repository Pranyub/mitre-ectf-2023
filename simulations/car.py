import serial

dev = '/dev/cu.usbmodem0E2349B71'

ser = serial.Serial(dev, 115200, timeout=1)

nonce_s = 0
nonce_c = 0
challege = []
challenge_resp = []

m = {}

def parse_msg(msg_raw):
    msg_raw = msg_raw[10:]
    m.target = msg_raw[0]
    m.magic = msg_raw[1]
    m.nonce_c = msg_raw[2:10]
    m.nonce_s = msg_raw[10:18]
    m.payload_size = msg_raw[18:22]
    m.payload_buf = msg_raw[22:534]
    m.payload_hash = msg_raw[534:566]

def send_chall(msg):
    pass