
FROM ubuntu

RUN apt-get update && apt-get upgrade -y && apt-get install -y \
    make \
    python3.9 \
    python-pip \
    clang \
    binutils-arm-none-eabi \
    gcc-arm-none-eabi


RUN python3 -m pip install pycryptodome