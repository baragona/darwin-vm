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
    for byte in command:
        connection.sendall(bytes([byte]))
        time.sleep(0.04)
    deadline = time.monotonic() + args.timeout
    recent = b''
    while time.monotonic() < deadline:
        if not select.select([connection], [], [], 1)[0]:
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
