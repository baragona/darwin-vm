#!/usr/bin/env python3
"""Diagnostic: change the existing sepfw-load-at-boot u32 from 1 to 0.

Preserves every other byte in the patched tree. This advertises that no SEP
firmware is loaded; it does not emulate SEP or implement key-store services.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('source')
p.add_argument('destination')
args = p.parse_args()
data = bytearray(Path(args.source).read_bytes())
source_hash = hashlib.sha256(data).hexdigest()
key = b'sepfw-load-at-boot'.ljust(32, b'\0')
if data.count(key) != 1:
    raise SystemExit('Expected exactly one sepfw-load-at-boot property')
offset = data.index(key)
if struct.unpack_from('<II', data, offset + 32) != (4, 1):
    raise SystemExit('Expected a 4-byte property with value 1')
struct.pack_into('<I', data, offset + 36, 0)
with Path(args.destination).open('xb') as output:
    output.write(data)
print(json.dumps({'source_sha256': source_hash, 'patched_sha256': hashlib.sha256(data).hexdigest(),
                  'property': 'sepfw-load-at-boot', 'value_offset': hex(offset + 36),
                  'before': 1, 'after': 0}, indent=2))
