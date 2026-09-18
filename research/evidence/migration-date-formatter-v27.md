# Migration bookkeeping narrows to date-formatter initialization (v27)

v26 and v27 eventually hit an SPTM panic after repeated service crashes.
Both were stopped only after their serial logs reported an unrecoverable
nested panic. The relation between those kernel panics and the helper crashes
is unknown. SPTM reported PC `0xfffffe00071024e0`, LR
`0xfffffe0007101ff4`; nested kernel abort PC `0xfffffe002ad5b344`.

Fresh v27 reproduced the helper SIGSEGV. Its shared-cache slide was
`0x16210000`; all addresses below are unslid for 24A437.

The third enumeration in `performSynchronousBuildUpgradeMigration:context:error:`
completed: breakpoint `0x206fd5dfc` hit. The callback at `0x206fd6228`
calls `_excludeContainersFromBackupWithContext:containerConfig:`; that method
returned for the observed configurations. No patch to skip backup or migration
was applied.

Execution reached `0x206fd5e04` and then `0x206fd5e20`, which calls
`setMigrationCompleteForType:`. Within that method:

- Entry at `0x206fd272c` hit.
- `0x206fd2774` hit after obtaining the current date.
- `0x206fd2784`, after `_iso8601DateFormatter`, did not hit before SIGSEGV.
- Consequently the date-to-string conversion at `0x206fd2790` was not reached.

This narrows the fault to formatter initialization, not the backup callback
or generic block cleanup. The formatter's once block starts at `0x206fd2da0`;
it constructs the formatter and sets options `0x773`. The exact faulting
instruction inside Foundation/ICU has not been captured.

Host inspection of the detached v27 guest image confirmed `/usr/share/icu`
was absent. The matching full system image has `icudt78l.dat` and
`icutzformat.txt`. v28 adds only those data files, with source/destination
hash verification recorded in `icu-resources-v28.json`. This is a controlled
resource experiment; its result must be assessed from v28, not inferred from
the missing directory alone.

All v27 debugger breakpoints were deleted and LLDB detached before stopping
the guest and mounting its disposable image. Reproduction staging commands:

```sh
# With the research VM stopped and service-root.dmg mounted at this path:
cp -R /tmp/a19-system/usr/share/icu /tmp/a19-service-edit/usr/share/icu
hdiutil detach /tmp/a19-service-edit
```

The actual staging used Python copytree and SHA-256 verification; only hash
metadata and notes are committed, not Apple's resource files.
