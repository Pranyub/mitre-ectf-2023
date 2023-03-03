python3 -m ectf_tools device.load_hw --dev-in ./fobout --dev-name fobf1 --dev-serial /dev/cu.usbmodem0E2349B71 & \
python3 -m ectf_tools device.load_hw --dev-in ./carout --dev-name carf1 --dev-serial /dev/cu.usbmodem0E2349AA1 \
&& fg


#python3 -m ectf_tools device.load_hw --dev-in ./ufobout --dev-name unpairedfob --dev-serial /dev/cu.usbmodem0E2349AA1 \
#python3 -m ectf_tools device.load_hw --dev-in ./carout --dev-name carf1 --dev-serial /dev/cu.usbmodem0E2349B71
#python3 -m ectf_tools run.package --name stablev2 --deployment featuretest --package-out packageout --package-name pak1 --car-id 1 --feature-number 1
#python3 -m ectf_tools run.enable --name stablev2 --package-in packageout --package-name pak1 --fob-bridge 1
