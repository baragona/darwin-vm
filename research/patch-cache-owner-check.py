#!/usr/bin/env python3
"""Diagnostic only: bypass the shared-cache root-owner check in 24A437.

Requires the exact output of patch-memdev-block-size.py. This weakens guest
cache admission and is NOT a production fix; correct image ownership should
replace it. All other shared-region checks remain intact.
"""
import argparse
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source')
parser.add_argument('destination')
args = parser.parse_args()
expected = 'dc220ea38e28fac5495a4147dea69888ca5971727fad9076c30d237232d145bb'
data = bytearray(Path(args.source).read_bytes())
if hashlib.sha256(data).hexdigest() != expected:
    raise SystemExit('Expected exact 24A437 collection with only the memdev patch')
offset = 0x41a8760
before, after = bytes.fromhex('c8400035'), bytes.fromhex('1f2003d5')
if data[offset:offset + 4] != before:
    raise SystemExit('Unexpected shared-region owner-check instruction')
data[offset:offset + 4] = after
with Path(args.destination).open('xb') as output:
    output.write(data)
print(json.dumps({'source_sha256': expected,
                  'patched_sha256': hashlib.sha256(data).hexdigest(),
                  'offset': hex(offset), 'before': before.hex(), 'after': after.hex(),
                  'description': 'Replace cbnz w8 (va_uid), failure with nop'}, indent=2))
