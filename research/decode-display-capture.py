#!/usr/bin/env python3
"""Decode display-capture-probe's BGRA UART export to PNG.

Export with: echo DISPLAY_DATA_BEGIN; /bin/base64
/private/var/tmp/display-capture.bgra; echo DISPLAY_DATA_END
The PNG keeps the original row order. No image content is synthesized.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import re
import struct
import zlib


def chunk(kind, data):
    return (struct.pack('>I', len(data)) + kind + data
            + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('transcript', type=Path)
    parser.add_argument('output_prefix', type=Path)
    parser.add_argument('--zlib', action='store_true',
                        help='decode the lossless display-pack-probe output')
    parser.add_argument('--width', type=int, default=416,
                        help='captured mode width; legacy default 416')
    parser.add_argument('--height', type=int, default=496,
                        help='captured mode height; legacy default 496')
    args = parser.parse_args()
    if not (0 < args.width <= 4096 and 0 < args.height <= 4096
            and args.width * args.height <= 4 * 1024 * 1024):
        parser.error('dimensions must be positive and fit a 16 MiB BGRA frame')
    lines = args.transcript.read_text().splitlines()
    begin = lines.index('DISPLAY_DATA_BEGIN')
    end = lines.index('DISPLAY_DATA_END', begin + 1)
    encoded = ''.join(line for line in lines[begin + 1:end]
                      if re.fullmatch(r'[A-Za-z0-9+/]+={0,2}', line))
    data = base64.b64decode(encoded, validate=True)
    width, height = args.width, args.height
    if args.zlib:
        unpacker = zlib.decompressobj()
        data = unpacker.decompress(data, width * height * 4 + 1)
        if not unpacker.eof or unpacker.unused_data or unpacker.unconsumed_tail:
            raise ValueError('Incomplete, oversized, or trailing zlib data')
    if len(data) != width * height * 4:
        raise ValueError(f'Expected {width * height * 4} bytes, got {len(data)}')
    report = dict(width=width, height=height, format='BGRA', bytes=len(data),
                  sha256=hashlib.sha256(data).hexdigest(),
                  colored_pixels=sum(any(data[i:i + 3])
                                     for i in range(0, len(data), 4)),
                  opaque_pixels=sum(data[i] == 255
                                    for i in range(3, len(data), 4)),
                  orientation='raw row order; no transform')
    rgba = b''.join(bytes((data[i + 2], data[i + 1], data[i], data[i + 3]))
                    for i in range(0, len(data), 4))
    row = width * 4
    scanlines = b''.join(b'\0' + rgba[y * row:(y + 1) * row]
                         for y in range(height))
    png = (b'\x89PNG\r\n\x1a\n'
           + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(scanlines)) + chunk(b'IEND', b''))
    Path(f'{args.output_prefix}.png').write_bytes(png)
    Path(f'{args.output_prefix}.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
