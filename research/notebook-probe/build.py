#!/usr/bin/env python3
"""Build the offline UIKit notebook with a locally supplied iOS libSystem stub."""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import re
import struct
import shutil
import subprocess
import uuid

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--libsystem', type=Path, required=True)
p.add_argument('--output', type=Path, required=True, help='new output directory')
p.add_argument('--trustcache', type=Path, help='version-1 guest cache to extend')
p.add_argument('--image', type=Path, required=True, help='local PNG to display in the notebook')
a = p.parse_args()
if a.image.read_bytes()[:8] != b'\x89PNG\r\n\x1a\n':
    p.error('--image must be a PNG')
a.output.mkdir(parents=True, exist_ok=False)
bundle = a.output / 'Notebook.app'
bundle.mkdir()
shutil.copyfile(a.image, bundle / 'Photo.png')
info = {
    'CFBundleIdentifier': 'org.baragona.Notebook',
    'CFBundleExecutable': 'Notebook', 'CFBundleName': 'Field Notes',
    'CFBundleDisplayName': 'Field Notes', 'CFBundlePackageType': 'APPL',
    'CFBundleVersion': '1', 'CFBundleShortVersionString': '1.0',
    'CFBundleIconFiles': ['Photo.png'],
    'CFBundleSupportedPlatforms': ['iPhoneOS'], 'MinimumOSVersion': '27.0',
    'UIDeviceFamily': [1], 'LSRequiresIPhoneOS': True,
    'UILaunchScreen': {}, 'UISupportedInterfaceOrientations': ['UIInterfaceOrientationPortrait'],
    'UIApplicationSceneManifest': {
        'UIApplicationSupportsMultipleScenes': False,
        'UISceneConfigurations': {'UIWindowSceneSessionRoleApplication': [{
            'UISceneConfigurationName': 'Default',
            'UISceneClassName': 'UIWindowScene',
            'UISceneDelegateClassName': 'NotebookSceneDelegate',
        }]},
    },
}
(bundle / 'Info.plist').write_bytes(plistlib.dumps(info))
source = Path(__file__).with_name('main.c')
exe = bundle / 'Notebook'
subprocess.run(['xcrun', 'clang', '-Wall', '-Wextra', '-Werror',
                '-Wno-incompatible-sysroot', '-target', 'arm64-apple-ios27.0',
                '-nostdlib', str(source), str(a.libsystem), '-o', str(exe)], check=True)
subprocess.run(['codesign', '-s', '-', str(bundle)], check=True)
subprocess.run(['codesign', '--verify', '--strict', str(bundle)], check=True)
identity = subprocess.run(['codesign', '-d', '--verbose=4', str(exe)],
                          check=True, capture_output=True, text=True).stderr
cdhash = re.search(r'^CDHash=([0-9a-f]{40})$', identity, re.M)[1]
report = {'bundle_id': info['CFBundleIdentifier'], 'cdhash': cdhash,
          'binary_sha256': hashlib.sha256(exe.read_bytes()).hexdigest(),
          'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
          'image_sha256': hashlib.sha256(a.image.read_bytes()).hexdigest(),
          'runtime_verified': False}
if a.trustcache:
    data = a.trustcache.read_bytes()
    version, = struct.unpack_from('<I', data)
    count, = struct.unpack_from('<I', data, 20)
    if version != 1 or len(data) != 24 + count * 22:
        raise ValueError('expected exact version-1 trust cache')
    entries = [data[i:i+22] for i in range(24, len(data), 22)]
    entry = bytes.fromhex(cdhash) + bytes([2, 0])
    if entry not in entries:
        entries.append(entry)
    entries.sort()
    (a.output / 'notebook.tc').write_bytes(
        struct.pack('<I', 1) + uuid.uuid4().bytes + struct.pack('<I', len(entries)) + b''.join(entries))
    report.update(trustcache_entries_before=count, trustcache_entries_after=len(entries))
(a.output / 'build.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
