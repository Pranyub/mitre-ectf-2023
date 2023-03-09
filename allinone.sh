python3 -m ectf_tools --debug build.env --design . --name stable4
python3 -m ectf_tools --debug build.tools --design . --name stable4
python3 -m ectf_tools --debug build.depl --design . --name stable4 --deployment test
python3 -m ectf_tools build.car_fob_pair --design . --name stable4 --deployment test --car-out car1 --fob-out pfob1 --car-name car --fob-name pfob --car-id 0 --pair-pin 123456
python3 -m ectf_tools build.fob --design . --name stable4 --deployment test --fob-out ufob1 --fob-name ufob

python3 -m ectf_tools run.package --name stable4 --deployment test --package-out packages --package-name stable4_1 --car-id 0 --feature-number 1
python3 -m ectf_tools --debug run.enable --name stable4 --fob-bridge 1 --package-in packages --package-name stable4_1
