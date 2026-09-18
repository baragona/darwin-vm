#!/usr/bin/env python3
"""Send one QMP command to an experimental VM (no third-party dependencies)."""
import argparse
import json
import socket


def execute(path, command, arguments):
    with socket.socket(socket.AF_UNIX) as connection:
        connection.settimeout(15)
        connection.connect(path)
        stream = connection.makefile('rwb')
        greeting = json.loads(stream.readline())
        if 'QMP' not in greeting:
            raise RuntimeError(greeting)
        for ident, name, args in [(1, 'qmp_capabilities', {}),
                                   (2, command, arguments)]:
            stream.write((json.dumps({'execute': name, 'arguments': args,
                                      'id': ident}) + '\n').encode())
            stream.flush()
            while True:
                raw = stream.readline()
                if not raw:
                    raise RuntimeError('QMP disconnected before replying')
                reply = json.loads(raw)
                if reply.get('id') == ident:
                    if 'error' in reply:
                        raise RuntimeError(reply['error'])
                    break
        return reply['return']


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('socket')
    parser.add_argument('command')
    parser.add_argument('arguments', nargs='?', default='{}', type=json.loads)
    args = parser.parse_args()
    print(json.dumps(execute(args.socket, args.command, args.arguments), indent=2))
