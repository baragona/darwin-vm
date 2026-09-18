#!/usr/bin/env python3
"""Compare display/input device matching in original and patched Apple trees."""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import dt_fixup


def load(path):
    root = dt_fixup.ADTNode()
    dt_fixup.decode_node(Path(path).read_bytes(), root)
    return dict(walk(root))


def walk(node, parent=''):
    path = parent + '/' + node.props['name']
    yield path, node
    for child in node.children:
        yield from walk(child, path)


def compatible(node):
    value = node.props.get('compatible')
    if isinstance(value, bytes):
        return value.rstrip(b'\0').decode('utf-8', errors='replace').split('\0')
    return [value] if value is not None else []


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('original')
parser.add_argument('patched')
args = parser.parse_args()
original, patched = load(args.original), load(args.patched)
result = []
for path, node in original.items():
    before = compatible(node)
    if not re.search(r'display|clcd|dcp|agx|gpu|sgx|multitouch|exdisp|mipi',
                     path + ' ' + ' '.join(before), re.I):
        continue
    after = compatible(patched[path]) if path in patched else None
    result.append({'path': path, 'original_compatible': before,
                   'patched_compatible': after,
                   'state': 'node removed' if after is None else
                   'matching removed' if before and not after else 'retained'})
print(json.dumps(result, indent=2))
