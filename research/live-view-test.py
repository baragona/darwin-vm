#!/usr/bin/env python3
"""Protocol integrity tests; no guest or host HID access."""
import base64
import collections
import importlib.util
from pathlib import Path
import struct
import threading
import unittest
import zlib

spec = importlib.util.spec_from_file_location('viewer', Path(__file__).with_name('live-view.py'))
v = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)


def frame(raw, packed=None):
    packed = zlib.compress(raw) if packed is None else packed
    return [f'LIVE_FRAME_BEGIN 1 2 1 8 {len(packed)} {zlib.crc32(packed):08x}'.encode(),
            b'LIVE_FRAME_DATA 1 0 ' + base64.b64encode(packed), b'LIVE_FRAME_END 1']


class Protocol(unittest.TestCase):
    def test_late_ack_keeps_writer_alive_without_resending(self):
        bridge=v.Bridge.__new__(v.Bridge)
        bridge.lock=threading.Lock();bridge.queue=collections.deque(['P'])
        bridge.ready=threading.Event();bridge.ready.set()
        bridge.stopped=False;bridge.status='Connected';bridge.version=3
        bridge.parser=v.Frames();sent=[]
        bridge.send=lambda text:sent.append(text)
        class DelayedAck:
            waits=0
            def clear(self):pass
            def wait(self,timeout):
                self.waits+=1
                if self.waits<=2:return False
                if self.waits==3:
                    self_status=bridge.status
                    assert self_status=='Waiting for guest acknowledgement'
                    assert not bridge.stopped and sent==['P\n']
                    bridge.queue.append('R')
                    return True
                bridge.stopped=True
                return True
        bridge.ack=DelayedAck();bridge.writer()
        self.assertEqual(sent,['P\n','R\n'])
        self.assertEqual(bridge.ack.waits,4)

    def test_actual_pixels_survive_png_conversion(self):
        parser = v.Frames()
        raw = bytes([1, 2, 3, 255, 4, 5, 6, 255])
        for line in frame(raw):
            result = parser.line(line)
        seq, png = result
        self.assertEqual(seq, 1)
        self.assertEqual(png[:8], b'\x89PNG\r\n\x1a\n')
        pos = 8
        pixels = b''
        while pos < len(png):
            size = struct.unpack('>I', png[pos:pos+4])[0]
            kind, data = png[pos+4:pos+8], png[pos+8:pos+8+size]
            self.assertEqual(zlib.crc32(kind+data), struct.unpack('>I', png[pos+8+size:pos+12+size])[0])
            if kind == b'IDAT':
                pixels += data
            pos += size + 12
        self.assertEqual(zlib.decompress(pixels), bytes([0, 3, 2, 1, 255, 6, 5, 4, 255]))

    def test_corruption_truncation_and_wrong_sequence_rejected(self):
        original = frame(b'12345678')
        variants = [
            [original[0], original[1].replace(b'1 0 ', b'2 0 '), original[2]],
            [original[0], original[1].replace(b'1 0 ', b'1 1 '), original[2]],
            [original[0], original[2]],
            [original[0], original[1][:-1]+b'!', original[2]],
            [original[0].rsplit(b' ', 1)[0]+b' 00000000', *original[1:]],
            frame(b'', zlib.compress(b'x'*100000)),
            frame(b'', zlib.compress(b'12345678')+b'extra'),
        ]
        for lines in variants:
            with self.subTest(lines=lines):
                parser = v.Frames()
                for line in lines:
                    self.assertIsNone(parser.line(line))
                self.assertGreater(parser.rejected, 0)
                for line in original:
                    result = parser.line(line)
                self.assertIsNotNone(result)

    def test_patch_and_missing_base_recovery(self):
        parser = v.Frames()
        original = bytes([1,2,3,255,4,5,6,255])
        for line in frame(original):parser.line(line)
        replacement = bytes([9,8,7,255])
        packed = zlib.compress(replacement)
        patch = [f'LIVE_PATCH_BEGIN 2 1 2 1 1 0 1 1 4 {len(packed)} {zlib.crc32(packed):08x}'.encode(),
                 b'LIVE_FRAME_DATA 2 0 '+base64.b64encode(packed), b'LIVE_FRAME_END 2']
        for line in patch:result = parser.line(line)
        self.assertEqual(result[0], 2)
        self.assertEqual(parser.canvas, original[:4]+replacement)
        self.assertFalse(parser.needs_keyframe)
        self.assertEqual(parser.line(b'LIVE_FRAME_SAME 2'), result)
        # A missing intermediate frame must never patch the wrong pixels.
        before = parser.canvas
        parser.line(patch[0].replace(b'2 1 ', b'4 3 ', 1))
        self.assertTrue(parser.needs_keyframe)
        self.assertEqual(parser.canvas, before)
        self.assertIsNone(parser.line(b'LIVE_FRAME_SAME 4'))
        for line in frame(original):result = parser.line(line)
        self.assertFalse(parser.needs_keyframe)
        self.assertEqual(parser.canvas, original)
        # A truncated patch leaves the displayed canvas untouched.
        parser.line(patch[0]);parser.line(patch[2])
        self.assertEqual(parser.canvas, original)
        self.assertTrue(parser.needs_keyframe)

    def test_queue_coalesces_movement_preserves_release(self):
        bridge = v.Bridge.__new__(v.Bridge)
        bridge.ready = threading.Event();bridge.ready.set()
        bridge.stopped = False;bridge.version=2;bridge.lock = threading.Lock();bridge.queue = collections.deque()
        with self.assertRaises(ValueError):bridge.enqueue(['T 0.2 0.3'])
        bridge.version=3;bridge.enqueue(['T 0.2 0.3'])
        self.assertEqual(list(bridge.queue),['T 0.2 0.3']);bridge.queue.clear()
        with self.assertRaises(ValueError):bridge.enqueue(['S 0.8 0.5 0.2 0.5'])
        bridge.version=4;bridge.enqueue(['S 0.8 0.5 0.2 0.5'])
        self.assertEqual(list(bridge.queue),['S 0.8 0.5 0.2 0.5']);bridge.queue.clear()
        bridge.enqueue(['D 0 0', 'M 0.1 0.1', 'M 0.2 0.2', 'U 0.2 0.2'])
        self.assertEqual(list(bridge.queue), ['D 0 0', 'M 0.1 0.1', 'M 0.2 0.2', 'U 0.2 0.2'])
        for invalid in [['K 3 1'], ['D nan 0'], ['Q'], ['F'], ['K 4 1', 'bad']]:
            before = list(bridge.queue)
            with self.assertRaises(ValueError):bridge.enqueue(invalid)
            self.assertEqual(list(bridge.queue), before)
        bridge.queue.clear()
        bridge.enqueue(['D 0 0']+['M 0.1 0.1']*30)
        bridge.enqueue(['M 0.9 0.9', 'U 0.9 0.9'])
        self.assertEqual(len(bridge.queue),18)
        self.assertEqual(list(bridge.queue)[-2:], ['M 0.9 0.9','U 0.9 0.9'])
        bridge.stopped = True
        with self.assertRaises(ValueError):bridge.enqueue(['H'])

if __name__ == '__main__':unittest.main()
