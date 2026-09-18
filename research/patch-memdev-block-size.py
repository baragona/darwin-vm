#!/usr/bin/env python3
"""Experimental block-device size fix for the exact 24A437/T8150 bootkc.

XNU memdev.c shifts a uint32_t page count before widening to a byte count.
For an 8 GiB image this wraps to zero. Widen the block-count ioctl arithmetic
and block-strategy bounds calculation. This is not a complete memdev fix:
raw character I/O and core-dump range reporting are deliberately untouched.
The input hash and every instruction are checked; the source is never changed.
"""
import argparse
import hashlib
import json
from pathlib import Path

EXPECTED_SHA256 = '81280162c11eec579d4cfb377a96a5f9cdf4396eed2a7f0213e5072e1e5e0e0f'
PATCHES = [
    # File offsets in the complete kernel collection, not the extracted kernel.
    (0x3d7ed84, '0931090b', '0931098b', 'DKIOCGETBLOCKCOUNT: add x9, x8, x9, lsl #12'),
    (0x3d7ed88, '29050051', '290500d1', 'DKIOCGETBLOCKCOUNT: sub x9, x9, #1'),
    (0x3d7ed8c, '2809c81a', '2809c89a', 'DKIOCGETBLOCKCOUNT: udiv x8, x9, x8'),
    (0x3d7f004, '294d1453', '29cd74d3', 'mdevstrategy: lsl x9, x9, #12'),
]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source')
parser.add_argument('destination')
args = parser.parse_args()
data = bytearray(Path(args.source).read_bytes())
if hashlib.sha256(data).hexdigest() != EXPECTED_SHA256:
    raise SystemExit('Refusing to patch: not the expected original 24A437/T8150 collection')
for offset, before, after, description in PATCHES:
    if data[offset:offset + 4] != bytes.fromhex(before):
        raise SystemExit(f'Unexpected instruction at {offset:#x}')
    data[offset:offset + 4] = bytes.fromhex(after)
with Path(args.destination).open('xb') as output:
    output.write(data)
print(json.dumps({'source_sha256': EXPECTED_SHA256,
                  'patched_sha256': hashlib.sha256(data).hexdigest(),
                  'changes': [{'offset': hex(o), 'before': b, 'after': a,
                               'description': d} for o, b, a, d in PATCHES]}, indent=2))
