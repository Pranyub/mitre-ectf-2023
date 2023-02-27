#!/usr/bin/python3 -u

# @file gen_secret
# @author Jake Grycel
# @brief Example script to generate header containing secrets for the fob
# @date 2023
#
# This source file is part of an example system for MITRE's 2023 Embedded CTF (eCTF).
# This code is being provided only for educational purposes for the 2023 MITRE eCTF
# competition, and may not meet MITRE standards for quality. Use this code at your
# own risk!
#
# @copyright Copyright (c) 2023 The MITRE Corporation

import json
import argparse
import secrets
from pathlib import Path

def main():

    debug = True

    parser = argparse.ArgumentParser()
    parser.add_argument("--car-id", type=int)
    parser.add_argument("--pair-pin", type=str)
    parser.add_argument("--secret-file", type=Path)
    parser.add_argument("--header-file", type=Path)
    parser.add_argument("--paired", action="store_true")
    args = parser.parse_args()

    print(args.secret_file)

    f = open(args.secret_file, "r")
    factory_secrets = json.load(fp)

    dev_entropy = '{' + str([x for x in secrets.token_bytes(16)])[1:-1] + '}' #16 byte array

    paired_secrets = f'''
    #ifndef __FOB_SECRETS__
    #define __FOB_SECRETS__
    
    #define PAIRED 1
    #define SEC_PAIR_PIN {args.pair_pin}
    #define SEC_CAR_ID {args.car_id}
    #define SEC_PAIR_SECRET {factory_secrets['car'+args.car_id]}
    #define SEC_FACTORY_PUB {factory_secrets['pubkey']}
    #define SEC_FACTORY_ENTROPY {dev_entropy}

    #endif
    '''


    if args.paired:

        # Write to header file
        with open(args.header_file, "w") as fp:
            fp.write(paired_secrets)
    else:
        # Write to header file
        with open(args.header_file, "w") as fp:
            fp.write("#ifndef __FOB_SECRETS__\n")
            fp.write("#define __FOB_SECRETS__\n\n")
            fp.write("#define PAIRED 0\n")
            fp.write('#define PAIR_PIN "000000"\n')
            fp.write('#define CAR_ID "000000"\n')
            fp.write('#define CAR_SECRET "000000"\n\n')
            fp.write('#define PASSWORD "unlock"\n\n')
            fp.write("#endif\n")


if __name__ == "__main__":
    main()
