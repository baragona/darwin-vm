#!/usr/bin/env python3
"""24A437 diagnostic: honor aks-endpoint=0 before waiting for SEPManager.

Requires the exact cache-owner diagnostic bootkc. Does not emulate SEP;
only reorders the existing disable check ahead of the blocking lookup.
"""
import argparse
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source')
parser.add_argument('destination')
args = parser.parse_args()
expected = '77fc882042df1c309a6f47ebb893bcf73dd4af19e3f0a5198f195f71bf0a727d'
data = bytearray(Path(args.source).read_bytes())
if hashlib.sha256(data).hexdigest() != expected:
    raise SystemExit('Expected exact 24A437 cache-owner diagnostic collection')
offset = 0x25c1df8
before = bytes.fromhex('85a900941300805288f241b9e8030035')
after = bytes.fromhex('1300805288f241b90804003582a90094')
if data[offset:offset + len(before)] != before:
    raise SystemExit('Unexpected AKS instruction sequence')
data[offset:offset + len(before)] = after
with Path(args.destination).open('xb') as output:
    output.write(data)
print(json.dumps({'source_sha256': expected,
                  'patched_sha256': hashlib.sha256(data).hexdigest(),
                  'offset': hex(offset), 'before': before.hex(), 'after': after.hex(),
                  'description': 'Check endpoint-disable field before SEPManager lookup'}, indent=2))
