#!/usr/bin/env python3
"""Experimental 24A437 SPTM substitution; NOT a verified opcode implementation.

At ASID-generation mismatch, replace unknown Apple opcode 0x0020134d with
architectural TLBI VMALLE1IS. The full EL1 invalidation is a diagnostic
hypothesis, not evidence of equivalence to Apple's instruction.
"""
import argparse
import hashlib
import struct
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('input', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
b = bytearray(args.input.read_bytes())
# File offset corresponds to unslid VA 0xfffffff0271024e0 in this SPTM.
offset = 0xfe4e0
original = bytes.fromhex('4d132000')
context = bytes.fromhex('196929386a0a4079dffeff17')
if b[offset-12:offset] != context or b[offset:offset+4] != original:
    raise SystemExit('Unsupported SPTM: instruction or surrounding code differs')
old_hash = hashlib.sha256(b).hexdigest()
if old_hash != 'd4b9a9db5d383734585a075da7afdd38a72ec7f5d43dbbd54708f07841fd5734':
    raise SystemExit('Unsupported SPTM: SHA-256 differs from the inspected image')
b[offset:offset+4] = struct.pack('<I', 0xd508831f)  # tlbi vmalle1is
with args.output.open('xb') as stream:
    stream.write(b)
print(f'input sha256={old_hash}')
print(f'output sha256={hashlib.sha256(b).hexdigest()}')
print(f'offset={offset:#x}: 0020134d -> d508831f (experimental full EL1 TLB invalidation)')
