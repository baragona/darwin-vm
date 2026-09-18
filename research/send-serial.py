#!/usr/bin/env python3
"""Send a short command slowly enough for the emulated UART FIFO.

Output is on the framebuffer when the guest boots with serial=2.
"""
import argparse
import socket
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('socket')
parser.add_argument('command')
args = parser.parse_args()
with socket.socket(socket.AF_UNIX) as connection:
    connection.connect(args.socket)
    for byte in (args.command + '\n').encode():
        connection.sendall(bytes([byte]))
        time.sleep(0.04)
