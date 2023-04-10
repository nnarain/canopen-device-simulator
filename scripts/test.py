#!/usr/bin/env python3

import time
import struct
import canopen
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument('-i', '--interface', default='vcan0')
parser.add_argument('-n', '--node-id', type=int, required=True)
parser.add_argument('-e', '--eds-file', required=True)

args = parser.parse_args()

network = canopen.Network()
network.connect(channel=args.interface, bustype='socketcan')

node = network.add_node(args.node_id, args.eds_file)

try:
    for num in range(0xFFFFFFFF):
        node.sdo.download(0x4000, 0x00, struct.pack('<I', num))
        time.sleep(0.001)
except KeyboardInterrupt:
    pass
