#!/usr/bin/env python3
"""Bounded live QEMU exception trace; retain breakpoint/undefined traps only.

Temporarily owns QEMU's log configuration. Drains other exception records through
an in-memory pipe so ordinary system calls do not fill disk. Does not stop CPUs
or change guest instructions. Must not run alongside another QEMU log consumer.
"""
import argparse
import collections
import json
import os
from pathlib import Path
import select
import tempfile
import threading
import time
from qmp import execute


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('socket')
    ap.add_argument('output')
    ap.add_argument('--seconds', type=float, default=120)
    ap.add_argument('--stop-file', type=Path, help='stop cleanly when this new file appears')
    args = ap.parse_args()
    if not 0 < args.seconds <= 600:
        ap.error('--seconds must be in (0, 600]')
    if args.stop_file and args.stop_file.exists():
        ap.error('--stop-file must not already exist')
    output = Path(args.output)
    counts = collections.Counter()
    stop = threading.Event()
    errors = []
    started = time.monotonic()
    with output.open('x') as report, tempfile.TemporaryDirectory(prefix='a19-traps-') as tmp:
        fifo = Path(tmp) / 'qemu.log'
        os.mkfifo(fifo)
        fd = os.open(fifo, os.O_RDWR | os.O_NONBLOCK)

        def emit(record):
            report.write(json.dumps(record) + '\n')
            report.flush()

        def drain():
            buf = b''
            record = []
            keep = False
            saved = 0
            try:
                while not stop.is_set() or select.select([fd], [], [], 0)[0]:
                    if not select.select([fd], [], [], .1)[0]:
                        continue
                    buf += os.read(fd, 65536)
                    while b'\n' in buf:
                        line, buf = buf.split(b'\n', 1)
                        line = line.decode(errors='replace')
                        if line.startswith('Taking exception '):
                            if keep and saved < 1000:
                                emit({'elapsed': time.monotonic()-started, 'lines': record})
                                saved += 1
                            counts[line] += 1
                            keep = '[Breakpoint]' in line or '[Undefined Instruction]' in line
                            record = [line] if keep else []
                        elif keep and len(record) < 32:
                            record.append(line)
                    if len(buf) > 65536:
                        raise RuntimeError('unexpected oversized QEMU log line')
                if keep and saved < 1000:
                    emit({'elapsed': time.monotonic()-started, 'lines': record})
            except Exception as exc:
                errors.append(repr(exc))

        def hmp(command):
            reply = execute(args.socket, 'human-monitor-command', {'command-line': command})
            if reply.strip():
                raise RuntimeError(reply)

        worker = threading.Thread(target=drain)
        worker.start()
        try:
            hmp('logfile ' + str(fifo))
            hmp('log int')
            emit({'event': 'start', 'unix_time': time.time(), 'monotonic': time.monotonic()})
            print('TRACE_READY', flush=True)
            deadline = time.monotonic() + args.seconds
            while (time.monotonic() < deadline and not errors
                   and not (args.stop_file and args.stop_file.exists())):
                time.sleep(min(.2, max(0, deadline-time.monotonic())))
        finally:
            # Keep draining while QMP disables logging: otherwise a full pipe can
            # block the vCPU and prevent the monitor from servicing this request.
            try:
                hmp('log none')
                hmp('logfile /tmp/a19-qemu-trace-disabled.log')
            finally:
                stop.set()
                worker.join()
                os.close(fd)
        emit({'summary': dict(counts), 'errors': errors,
              'elapsed': time.monotonic()-started, 'unix_time': time.time()})
        print('TRACE_FINISHED', dict(counts), errors, flush=True)
        if errors:
            raise RuntimeError(errors)


if __name__ == '__main__':
    main()
