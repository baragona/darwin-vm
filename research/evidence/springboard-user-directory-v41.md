# SpringBoard managed-job bad access (24A437, v41)

Caught with a hardware breakpoint at kernel address `0xfffffe002ab8e2a4`,
located by cross-references to the exception-triage diagnostic strings and
corroborated by its caller/arguments. The kernel slide is `0x20000000`.
Filter: `($x0 & 0x1ff) == 1 || $x0 == 6` (bad access or breakpoint exceptions).
A preliminary EXC_CRASH notification was continued; it was not identified as
SpringBoard's original fault.

At the managed SpringBoard fault:

- Exception type x0: 1 (bad access).
- Exception code array: `[1, 0x30]` (invalid address 0x30).
- Kernel caller LR: `0xfffffe002ad52f50`.
- LLDB unwound through the exception boundary to user frame 4.
- User PC: `0x19ac4c310`; x0: 0; instruction: `ldr x2, [x0, #0x30]`.
- User FP: `0x16b98ea30`; SP: `0x16b98ea20`.
- Shared-cache base: `0x18b370000`, verified by reading `dyld_v1  arm64e`.
- Shared-cache slide: `0xb370000`; unslid fault PC: `0x18f8dc310`.

The adjacent calls resolve to `getuid` and `getpwuid`. The latter's stub loads
its target from `0x1e8d13048`, which resolves to `0x18ffb613c`, the exported
`getpwuid` in libsystem_info. The preceding direct stub resolves to `getuid`
at `0x237efdee8`. The fault dereferences the returned passwd pointer's directory
field without checking for null.

Relevant user call chain (symbolication subtracts four from return addresses):

1. `___BSCurrentUserDirectory_block_invoke`
2. `_BSCurrentUserDirectory`
3. `ChronoServices.ChronoLibraryPath` getter
4. `-[CHSWidgetExtensionProvider initWithConnection:providerOptions:eduProvider:]`
5. `-[CHSWidgetExtensionProvider initWithOptions:]`
6. `-[SBChronoApplicationProcessStateObserver init]`
7. `+[SBChronoApplicationProcessStateObserver sharedInstance]`
8. `_SBSystemAppMain`

See `springboard-user-directory-v41-symbols.json` for the raw symbol mapping.
The current public XNU saved-state declaration used as background is
[Apple's thread_status.h](https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/mach/arm/thread_status.h).
The actual registers above came from LLDB's exception-boundary unwind, not
an assumed in-memory saved-state layout.

Guest observation: `/etc/passwd` is absent; root's `id mobile` works using the
protected master database. Both the mounted original restore and this guest
lack the public file, whereas the matched 24A437 system image contains it.
The master file already has the mobile account. The v42 experiment adds only
the matching public `/private/etc/passwd`, mode 0644, leaving master.passwd
unchanged. Its byte count and digest are in `public-user-database-v42.json`.

All breakpoints were deleted and LLDB detached before intentionally stopping
v41 for this resource update. No Developer Mode override was needed to catch
the kernel exception.
