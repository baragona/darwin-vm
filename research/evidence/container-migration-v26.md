# Writable-var exception and migration crash (v26)

The v25 helper failed `createPathsIfNecessaryWithError:` with EPERM because
its embedded sandbox resolved `/var` to `/mnt1`. For this diagnostic build,
all original helper entitlements were preserved and the absolute-path
read-write exception `/mnt1/` was added. The embedded `containermanagerd`
profile remains enabled. See `../container-path-entitlements.plist` and
`container-path-probe-build.json` for the signing inputs and hashes.
The ad-hoc signed helper is installed at its original guest path; its CDHash
was merged into the existing v1 trust cache (4384 entries).

## Observed improvement

The helper created root-owned directories under
`/var/root/Library/MobileContainerManager`: `Delete`, `System`,
`System/PendingUpdates`, and `System/Replace`.
At unslid PC `0x206f6cf28`, immediately after directory creation, LLDB
reported `x0=1` and the out-error at `[sp+0x60]` was zero. In v25 this
operation returned false with EPERM. Thus the exception fixes this specific
path denial; a change in exit signal alone is not the evidence.

## Remaining crash

The helper now exits with SIGSEGV. Launchd recorded this for PIDs 49 and 50
(and subsequent retries). The main startup block reached its return at
`0x206f6d2e8`. Initialization at `0x206f6d0f8` returned a non-null object
with a nil error; the next initializer also returned non-null.

The next block enters `-[MCMClientConnection rebootContainerManagerSetup]`.
`isBuildUpgrade` returned 1 at `0x206f6ab40`. Execution reached the call to
`performSynchronousBuildUpgradeMigration:context:error:` at `0x206f6ab84`.
Within that migration method, these breakpoints were hit in order:

- `0x206fd585c`
- `0x206fd58f8`
- `0x206fd5998`
- `0x206fd5c0c`
- `0x206fd5c5c`
- `0x206fd5cd8` (after the first block enumeration)
- `0x206fd5d48` (after the second block enumeration)

The breakpoint at `0x206fd5e38` was not reached before the helper exited.
The remaining region includes a third block enumeration, whose callback
starts at `0x206fd6228`, plus cleanup. This is a narrowed failure interval,
not proof of the exact faulting instruction or root cause. Next inspect
that callback and the call at `0x206fd6270`, or capture the Mach exception.
Do not skip migration or claim container startup works based on these results.

## Reproduction context

All addresses above are unslid shared-cache addresses for 24A437.
The v26 shared-cache slide is `0x3fdc000`; add it before setting hardware
breakpoints. LLDB connects through QEMU at `127.0.0.1:63426`.
QMP is `/tmp/a19-ui-v26-qmp.sock`, UART `/tmp/a19-ui-v26-serial.sock`.
The guest uses the existing `bootkc-aks-endpoint-order` and writable-var
bootstrap. No Developer Mode override was applied in v26.

Trigger a fresh UI request using `bash /bin/ui-probe` through
`research/serial-command.py`. Stock `launchctl` is absent from this image;
trial invocations returned command-not-found and did not trigger retries.
On-demand service retries can be throttled by launchd. An observation timeout
while a breakpoint pauses the VM is not guest termination.

At checkpoint, all LLDB breakpoints were removed, the debugger detached,
and QMP reported `status=running`, `running=true`. Graphical startup remains
unverified. This scoped entitlement is a diagnostic accommodation for the
symlinked tmpfs layout, not a replacement for a proper data volume.
