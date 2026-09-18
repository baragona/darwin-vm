# Backboardd abort, v20

Live shared-cache slide: 0x1a030000.
`ipsw dyld symaddr` resolved `_abort` at unslid 0x187f788b0 and
`_objc_exception_throw` at unslid 0x180410328 in the exact 24A437 cache.
QEMU hardware breakpoints at their slid addresses both triggered during
managed backboardd startup requested by `/bin/display-probe`.

At the exception-throw breakpoint, LR was 0x19ab311f4. The backtrace included:

- 0x19ab88a58: +[NSURL fileURLWithPath:]
- 0x1a9948964: -[BSSystemContainerForCurrentProcessPathProvider libraryPath]
- 0x1a99489b4: -[BSSystemContainerForCurrentProcessPathProvider cachesPath]

The raw exception pointer in x0 was 0x0800007596c19020. Removing its top-byte
tag gave 0x7596c19020. Reads at object +8 and +16 yielded the name and reason
string objects. Name's backing string at 0x19aaf3d9d:

    NSInvalidArgumentException

Reason's inline bytes at 0x759701d5f1:

    *** -[NSURL initFileURLWithPath:]: nil string parameter

These are transcribed live debugger observations. Saved symbolication output
is adjacent; libc++ symbol aliases can be misleading, so the exception reason
and Foundation/BaseBoard chain are the relevant evidence. The missing
`com.apple.containermanagerd.system` lookup is consistent with the nil path,
but adding that service still requires a runtime test.

All hardware breakpoints were removed and LLDB detached before v20 was stopped
for the v21 image update. No exception or abort instructions were patched.
