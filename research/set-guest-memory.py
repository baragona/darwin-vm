#!/usr/bin/env python3
"""Change only dram-size in an already-patched device tree.

Avoid decode/re-encode: dt_fixup's string heuristic changes the 256-byte
all-'A' random-seed into a 257-byte NUL-terminated property on a second pass.
"""
import argparse
from pathlib import Path
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source')
parser.add_argument('destination')
parser.add_argument('gib', type=int, choices=(8, 16, 32))
args = parser.parse_args()
data = bytearray(Path(args.source).read_bytes())
key = b'dram-size'.ljust(32, b'\0')
if data.count(key) != 1:
    raise SystemExit('Expected exactly one dram-size property')
offset = data.index(key)
if struct.unpack_from('<I', data, offset + 32)[0] != 8:
    raise SystemExit('Expected an 8-byte dram-size property')
struct.pack_into('<Q', data, offset + 36, args.gib * 1024**3)
with Path(args.destination).open('xb') as output:
    output.write(data)
