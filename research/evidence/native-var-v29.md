# Native /private/var mount, v29

The original unmodified `/sbin/mount_tmpfs -s 536870912 /private/var`
succeeds when `/private/var` is a real directory. This is distinct from the
v24 test at `/private/var/tmp`, which was denied. Earlier results did not
establish that mounting directly at `/private/var` would fail.

Offline preparation: stop the VM, mount service-root.dmg, remove only the
`/private/var -> /mnt1` symlink, move the empty original `/mnt3` directory to
`/private/var` (preserving its original inode ownership), and install the
updated `init-writable-var.sh`. `/var-seed` remains the seed source. Unmount
the image before booting. In v29, the helper executable and trust cache were
unchanged from v28: the helper still had its scoped `/mnt1/` exception.

The bootstrap produced `writable-var`, `WRITABLE_VAR_READY`, and:

```text
tmpfs on /private/var (tmpfs, local, nosuid)
```

See `direct-var-mount-v29.txt`. The helper created a system container and
stayed alive. No `file-issue-extension` denial appeared in this run.
Backboardd survived the initial check but later aborted after about 38 seconds;
process survival alone was not treated as successful startup.

## Changed exception and display initialization

Shared-cache slide: `0x1fd14000`. At hardware breakpoint
`objc_exception_throw` (unslid `0x180410328`, live `0x1a0124328`), the exception
name was `NSInternalInconsistencyException`. The reason identified:

```text
error reading /System/Library/BackBoard/EventProcessorConfiguration.plist:
Error Domain=NSCocoaErrorDomain Code=260
NSUnderlyingError: NSPOSIXErrorDomain Code=2 "No such file or directory"
```

This is different from the previous nil-container-path exception. The matching
full firmware has this 1202-byte plist. v30 stages it and restores the original
container helper byte-for-byte, removing the scoped entitlement experiment.
Input hashes are in `native-var-v30-inputs.json`.

Thread samples before abort, symbolicated against the matching shared cache,
show QuartzCore `CAWindowServer _detectDisplays`, `serverWithOptions:`,
`CA::Render::Server::server_thread`, and IOMobileFramebuffer display-list
population. Raw samples and symbolication are adjacent. This proves execution
reaches these paths, not that a display was detected or a frame rendered.

Developer Mode was temporarily enabled in v29 using the previously verified
TXM-state pointer. Guest status read back as 1, and task inspection succeeded.
All debugger breakpoints were removed and LLDB detached before v29 stopped.

## Rejected alternative

Automatic approval review rejected an experiment removing the helper's embedded
sandbox profile and adding a no-sandbox entitlement, citing persistent security
weakening and insufficiently explicit authorization. No part of that rejected
build or trust-cache mutation executed. The native-path mount is the safer
alternative actually tested; no sandbox bypass was introduced.
