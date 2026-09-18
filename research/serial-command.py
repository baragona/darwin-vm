#!/usr/bin/env python3
"""Run a short diagnostic command on an idle guest Bash UART; capture its output.

Requires exclusive UART access and an already booted shell. Timeout does not
stop the guest. Do not use while another collector owns the socket.
"""
import argparse
from pathlib import Path
import select
import socket
import time
import uuid

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('socket')
parser.add_argument('transcript')
parser.add_argument('command')
parser.add_argument('--timeout', type=int, default=30)
args = parser.parse_args()
marker = ('SERIAL_DONE_' + uuid.uuid4().hex).encode()
command = args.command.encode() + b'; echo ' + marker + b'\n'
with socket.socket(socket.AF_UNIX) as connection, Path(args.transcript).open('xb') as log:
    connection.settimeout(10)
    connection.connect(args.socket)
    # Drain output while typing: QEMU's UART can back-pressure under daemon
    # logging, and sending the whole line first can lose input bytes.
    offset = 0
    next_byte = time.monotonic()
    deadline = next_byte + len(command) * 0.04 + args.timeout
    recent = b''
    while time.monotonic() < deadline:
        now = time.monotonic()
        if offset < len(command) and now >= next_byte:
            connection.sendall(command[offset:offset + 1])
            offset += 1
            next_byte = time.monotonic() + 0.04
            if offset == len(command):
                deadline = time.monotonic() + args.timeout
        wait = min(1, max(0, deadline - time.monotonic()))
        if offset < len(command):
            wait = min(wait, max(0, next_byte - time.monotonic()))
        if not select.select([connection], [], [], wait)[0]:
            continue
        data = connection.recv(65536)
        if not data:
            raise SystemExit('Guest UART closed before completion marker')
        log.write(data)
        log.flush()
        print(data.decode(errors='replace'), end='', flush=True)
        recent = (recent + data)[-65536:]
        if any(line == marker for line in recent.replace(b'\r', b'').split(b'\n')[:-1]):
            break
    else:
        raise SystemExit('Observation timed out; guest and command may still be running')
