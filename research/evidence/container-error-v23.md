# V23 configuration result

All 28 plist files in the exact 24A437 ContainerManagerCommon.framework were
copied, along with the other framework resources. Source/destination plist
bytes were compared and hashes recorded in container-config-resources.json.
The framework's System.plist includes Platform, which resolves further files.

The managed helper still exits and backboardd still aborts. At the permanent-
error listener breakpoint (0x206f6e2c8 + shared-cache slide 0x17884000):

- Block: 0x73dcc09860
- Captured MCMError: 0x73dcc10210
- Error type at +0x10: 0x5b (91)
- Exact-cache description table entry [91]: 0x21e8f9c31, `DURING_STARTUP`

This differs from v22's INVALID_CONFIG_FILE (149). It supports successful
configuration loading but does not identify the specific startup operation
that failed. No writable storage or graphical startup is established.
All breakpoints were removed and LLDB detached.

Entitlement comparison for the next mount experiment: the original restore
mount_tmpfs binary has no entitlements printed by codesign; mount_apfs has
com.apple.private.security.disk-device-access. The original restored_external
has that entitlement and com.apple.private.security.no-sandbox. Testing an
explicitly trusted diagnostic mount helper with these matching restore-service
entitlements is a next candidate, not yet a verified solution.
