#!/usr/bin/env python3
"""Verify both --scene frames exported with FRAME{0,1}_{BEGIN,END} markers.

Checks every pixel, including the erased old rectangle, and exports PNGs from
actual guest bytes. The destination's default orientation reverses layer Y.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import re
import struct
import zlib


def png_chunk(kind, data):
    return (struct.pack('>I', len(data)) + kind + data
            + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))


def png_from_bgra(data):
    rgb = b''.join(bytes((data[i + 2], data[i + 1], data[i]))
                   for i in range(0, len(data), 4))
    rows = b''.join(b'\0' + rgb[y * 192:(y + 1) * 192] for y in range(64))
    return (b'\x89PNG\r\n\x1a\n'
            + png_chunk(b'IHDR', struct.pack('>IIBBBBB', 64, 64, 8, 2, 0, 0, 0))
            + png_chunk(b'IDAT', zlib.compress(rows)) + png_chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('transcript', type=Path)
    parser.add_argument('output_prefix', type=Path)
    args = parser.parse_args()
    lines = args.transcript.read_text().splitlines()
    report = []
    frames = []
    for frame, (left, layer_y, color) in enumerate([
            (8, 12, bytes([128, 0, 127, 255])),
            (32, 28, bytes([0, 255, 0, 255]))]):
        start = lines.index(f'FRAME{frame}_BEGIN')
        end = lines.index(f'FRAME{frame}_END', start + 1)
        encoded = ''.join(line for line in lines[start + 1:end]
                          if re.fullmatch(r'[A-Za-z0-9+/]+={0,2}', line))
        data = base64.b64decode(encoded, validate=True)
        if len(data) != 16384:
            raise ValueError(f'Frame {frame}: incorrect byte count {len(data)}')
        top = 64 - layer_y - 16
        expected = b''.join(color if left <= x < left + 24 and top <= y < top + 16
                            else bytes([0, 0, 255, 255])
                            for y in range(64) for x in range(64))
        mismatches = sum(data[i:i + 4] != expected[i:i + 4]
                         for i in range(0, len(data), 4))
        if mismatches:
            raise ValueError(f'Frame {frame}: {mismatches} incorrect pixels')
        frames.append(data)
        report.append(dict(frame=frame, matching_pixels=4096,
                           rectangle_pixels=384, background_pixels=3712,
                           rectangle_bgra=color.hex(), raw_rectangle=[left, top, 24, 16],
                           sha256=hashlib.sha256(data).hexdigest()))
    # Save only after both frames pass, including all old-position pixels.
    for frame, data in enumerate(frames):
        Path(f'{args.output_prefix}-{frame}.bgra').write_bytes(data)
        Path(f'{args.output_prefix}-{frame}.png').write_bytes(png_from_bgra(data))
    Path(f'{args.output_prefix}-verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
