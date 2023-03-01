python3 -m ectf_tools device.load_hw --dev-in ./fobout --dev-name examplefobname --dev-serial /dev/cu.usbmodem0E2349B71 & \
python3 -m ectf_tools device.load_hw --dev-in ./carout --dev-name examplecarname --dev-serial /dev/cu.usbmodem0E2349AA1 \
&& fg
