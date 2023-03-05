(trap 'kill 0' SIGINT; \
python3 -m ectf_tools device.bridge --bridge-id 1 --dev-serial /dev/cu.usbmodem0E2349AA1 & \
python3 -m ectf_tools device.bridge --bridge-id 2 --dev-serial /dev/cu.usbmodem0E2349B71 \
)
