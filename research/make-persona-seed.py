#!/usr/bin/env python3
"""Generate an experimental iOS 27 (24A437) persona manifest for a RAM-disk guest.

Schema recovered from usermanagerd's UMDPersonaManifest readers. This is not
stock provisioning: UUIDs and kernel IDs below are synthetic test values, and
writing this file does not allocate kernel personas or initialize APFS volumes.
"""
import argparse
import plistlib
import uuid
from pathlib import Path

SYSTEM_USER = 'FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF'
NAMESPACE = uuid.UUID('09b988ac-d2d5-453c-bfa9-08e5c7235427')
MOBILE_USER = str(uuid.uuid5(NAMESPACE, 'mobile-user')).upper()


def blob(value):
    return plistlib.dumps(value, fmt=plistlib.FMT_BINARY, sort_keys=True)


def make_manifest():
    users = {}
    kernel_id = 1001
    # Type sets match constructors at 0x1000959fc and 0x10008fe88.
    for user, uid, kinds in ((SYSTEM_USER, 0, (3, 5)),
                             (MOBILE_USER, 501, (0, 3, 5, 4))):
        personas = []
        for kind in kinds:
            personas.append({
                'UserPersonaType': kind,
                'UserPersonaUniqueString': str(uuid.uuid5(
                    NAMESPACE, f'{user}/type-{kind}')).upper(),
                'UserPersonaUserODUUID': user,
                'UserPersonaID': kernel_id,
                'UserPersonaUserUID': uid,
                'UserPersonaUserGID': uid,
                'UserPersonaNickName': f'darwinvm-{uid}-{kind}',
                'UserPersonaBundleIDS': [],
                'UserPersonaObserverService': [],
                'UserPersonaOnDeletion': False,
                'UserPersonaDisabled': False,
                'UsePersonaGenerationID': 1,
            })
            kernel_id += 1
        users[user] = blob({'NUMENT': len(personas), 'BLOB': blob(personas)})
    return {'UsePersonaManifestVersion': 1, 'UsePersonaGenerationID': 1,
            'UserPersonaDictionary': users}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    # Exclusive creation avoids accidentally replacing a guest's existing state.
    with args.output.open('xb') as stream:
        plistlib.dump(make_manifest(), stream, fmt=plistlib.FMT_XML)
    print(f'{args.output}: 2 users, 6 synthetic personas; kernel state not provisioned')


if __name__ == '__main__':
    main()
