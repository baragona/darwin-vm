#!/usr/bin/env python3
"""Upload a file to an idle guest Bash UART and verify SHA-256 before publishing.

Requires exclusive UART access, base64, sha256sum, stty, mv, and chmod in /bin.
The destination must not exist. A failed transfer leaves a uniquely named staging
file, never an executable destination. On interruption, inspect the live guest
before retrying; a timeout does not stop the guest or cancel its pending input.
Terminal echo may remain disabled until the pending input is cancelled and
`/bin/stty echo` is run in the guest.
"""
import argparse
import base64
import hashlib
from pathlib import Path
import select
import shlex
import socket
import time
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('socket')
    parser.add_argument('source', type=Path)
    parser.add_argument('destination')
    parser.add_argument('transcript', type=Path)
    parser.add_argument('--mode', choices=['644', '755'], default='644')
    parser.add_argument('--bytes-per-second', type=int, default=200)
    args = parser.parse_args()
    if not args.destination.startswith('/') or '\n' in args.destination:
        parser.error('destination must be an absolute guest path without newlines')
    if args.bytes_per_second <= 0:
        parser.error('bytes-per-second must be positive')
    payload = args.source.read_bytes()
    digest = hashlib.sha256(payload).hexdigest()
    token = uuid.uuid4().hex
    ready, hashed, done, end, exists = [f'UPLOAD_{s}_{token}' for s in ('READY', 'HASHED', 'DONE', 'END', 'EXISTS')]
    staging = args.destination + '.upload-' + token
    q = shlex.quote
    recent = bytearray()
    with socket.socket(socket.AF_UNIX) as connection, args.transcript.open('xb') as log:
        connection.settimeout(10)
        connection.connect(args.socket)

        def receive(timeout):
            if select.select([connection], [], [], timeout)[0]:
                data = connection.recv(65536)
                if not data:
                    raise RuntimeError('guest UART disconnected')
                log.write(data)
                log.flush()
                recent.extend(data)
                del recent[:-262144]

        def send(data, rate):
            # Drain concurrent daemon logs continuously to avoid UART backpressure.
            # The emulated UART can drop bursts even when shell echo is off.
            step = 1
            for offset in range(0, len(data), step):
                part = data[offset:offset + step]
                connection.sendall(part)
                until = time.monotonic() + len(part) / rate
                while time.monotonic() < until:
                    receive(max(0, until - time.monotonic()))

        def wait_line(value, timeout=30, reject=None):
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                lines = bytes(recent).replace(b'\r', b'').split(b'\n')[:-1]
                if reject and reject.encode() in lines:
                    raise RuntimeError('guest destination already exists; no upload performed')
                if value.encode() in lines:
                    return
                receive(min(1, max(0, deadline - time.monotonic())))
            raise RuntimeError(f'timed out waiting for {value}; inspect guest before retrying')

        # Keep ordinary command typing at the established conservative rate.
        setup = (f'if [ ! -e {q(args.destination)} ]; then /bin/stty -echo; '
                 f'echo {ready}; else echo {exists}; fi\n')
        send(setup.encode(), 25)
        wait_line(ready, reject=exists)
        print(f'Guest ready; sending {len(payload)} bytes', flush=True)
        # Bash consumes the here-document with terminal echo disabled. Base64 is
        # alphabet-only and the random terminator cannot occur in the payload.
        script = f'/bin/base64 -d > {q(staging)} <<\'{end}\'\n'.encode()
        encoded = base64.encodebytes(payload)
        script += encoded + f'{end}\n/bin/sha256sum {q(staging)}; /bin/stty echo; echo {hashed}\n'.encode()
        recent.clear()
        send(script, args.bytes_per_second)
        wait_line(hashed, 60)
        lines = bytes(recent).replace(b'\r', b'').split(b'\n')
        expected = (digest + '  ' + staging).encode()
        if expected not in lines:
            raise RuntimeError(f'SHA-256 not verified; destination untouched, staging file: {staging}')
        recent.clear()
        # Publish only after the host has verified the guest's hash, and guard
        # against a destination that appeared during transfer.
        publish = (f'if [ ! -e {q(args.destination)} ]; then '
                   f'/bin/chmod {args.mode} {q(staging)} && '
                   f'/bin/mv -n {q(staging)} {q(args.destination)} && '
                   f'[ ! -e {q(staging)} ] && echo {done}; fi\n')
        send(publish.encode(), 25)
        wait_line(done)
        print(f'Uploaded {args.destination} SHA256={digest}', flush=True)


if __name__ == '__main__':
    main()
