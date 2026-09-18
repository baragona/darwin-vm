#!/usr/bin/env python3
"""Collect a boot transcript and run /bin/ui-probe only after the Bash prompt."""
import argparse
from pathlib import Path
import select
import socket
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('socket')
parser.add_argument('transcript')
parser.add_argument('--timeout', type=int, default=180)
parser.add_argument('--observe-only', action='store_true',
                    help='collect UART output without sending a shell command')
args = parser.parse_args()
sent = False
recent = b''
with socket.socket(socket.AF_UNIX) as connection, Path(args.transcript).open('xb') as log:
    connection.settimeout(10)
    connection.connect(args.socket)
    deadline = time.monotonic() + args.timeout
    while time.monotonic() < deadline:
        if not select.select([connection], [], [], 1)[0]:
            continue
        data = connection.recv(65536)
        if not data:
            raise SystemExit('Guest serial connection closed')
        log.write(data)
        log.flush()
        recent = (recent + data)[-65536:]
        if not args.observe_only and not sent and b'bash-5.3#' in recent:
            print('Guest shell ready; sending UI probe', flush=True)
            for byte in b'bash /bin/ui-probe\n':
                connection.sendall(bytes([byte]))
                time.sleep(0.04)
            sent = True
        if sent and b'\nUI_PROBE_END' in recent:
            print('Probe finished; inspect transcript for individual command results', flush=True)
            break
    else:
        if args.observe_only:
            print('Observation period ended; guest remains running for inspection', flush=True)
        else:
            raise SystemExit('Timed out; guest remains running for inspection')
