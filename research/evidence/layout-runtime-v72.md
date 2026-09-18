# Querying Setup's registration (24A437)

The V71 app-layout assertion is not explained by an absent Setup application
record. V72's new query mode returned `LS_IS_INSTALLED=1` and
`file:///Applications/Setup.app/` for `com.apple.purplebuddy` both before and after
the system-app registry rebuild. SpringBoard's record is also installed at its
original bundle path. The rebuild returned 1. See `layout-registration-v72.txt`.
These results establish registration, not successful application or scene launch.
As a negative control, `com.example.darwinvm.missing` returned an INVALID proxy,
`LS_IS_INSTALLED=0`, and a nil bundle URL (`layout-query-negative-v72.txt`).

`ls-registration-probe.c` now supports:

```sh
/bin/ls-rebuild-probe --query com.apple.purplebuddy
/bin/ls-rebuild-probe --query com.apple.springboard
```

Query mode does not register an application. The original no-argument query,
SpringBoard registration, and system rebuild modes are retained. Explicit
`--register-setup` is available for a missing Setup record; it was **not** used in
this run because the record was already present. Its bundle identifier was
verified against the matching firmware Info.plist. Registration still requires a
fresh query process to check the resulting record.

The updated probe builds with `-Wall -Wextra -Werror`, passes strict ad-hoc signature
verification, and executes in the guest. Its new CDHash was added to the v1 trust
cache (4,409 to 4,410 entries). The image also now includes the MobileGestalt probe
launch plist, so it can be submitted before SpringBoard. All installed artifacts
and hashes are recorded in `layout-image-v72.json`; the old image and trust cache
were backed up before editing.

## Ownership experiment

The guest root is `/dev/md0`, read-only APFS; `/private/var` is writable tmpfs.
`/sbin/mount -uw /` returned 77 with EPERM and left the root read-only. A narrowly
scoped host `sudo -n chown` against only the mounted guest extensionkitservice
bundle required a password and did not run. No ownership fix was applied. The
existing per-request stat-result override therefore remains part of UI bootstrap.

V71's stderr also confirms the failed restart path: Swift could not cast an
`_NSXPCDistantObject` to `ExtensionFoundation._EXDiscoveryServiceProtocol` after
the original extension service was rejected. That is separate from the later
app-layout exception reached after applying the required bootstrap override.

## V72 transition observation

Cache slide is 0x2e3c000. A read-only breakpoint at unslid 0x224b12930 (runtime
0x22794e930), conditional on w0 == 0, watches the failed membership check immediately
before V71's assertion. At this instruction x23 is the target layout and the
coordinator comes from stack offset 0x88. No layout membership result is overridden.
The Objective-C exception observer is also enabled.

Setup and MobileGestalt daemons start before SpringBoard in this run. The first
wake reported the display off. The follow-up wake/swipe command exceeded its
collector's observation window while the guest remained running; collection was
resumed passively without reissuing the command. The layout observer did not fire
during this interval. After repeated key-store logging and no probe completion,
the foreground input probe was explicitly interrupted to permit diagnostics;
the shell acknowledged `INPUT_PROBE_CANCELLED_FOR_DIAGNOSTICS`.

The subsequent successful thread samples covered SpringBoard PID 67 and
MobileGestaltHelper PID 60. SpringBoard's main thread was in UIKit asset lookup
from `UISystemKeyboardDockController configuredGlyphWithName:` while handling a
dictation-availability update. The helper's main thread was in its run loop.
This sample differs from V71's synchronous DPI wait; it does not establish an
asset-lookup loop or prove that key-store errors cause the current UI problem.
See `layout-stall-threads-v72.txt` and `layout-main-stack-v72.txt`.

The inspection byte was restored to zero and read back after stopping in a kernel
mapping. User-mode mappings did not permit that write. A temporary breakpoint at
an observed kernel instruction was removed immediately after restoration.
SpringBoard remains running with the read-only layout and exception observers.
