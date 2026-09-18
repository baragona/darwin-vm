# Complete UIKit resource test (V59, 24A437)

The previous guest's QMP socket was absent before starting V59. This image
contains all 136 verified UIKitCore resource files, including localized
collation dictionaries. [Resource addition manifest](uikit-resources-v59.json).

Cache slide 0x1f4ec000; UserManager PID 38 base 0x104778000; verified mobile
NSString 0x7b0ac0c500. Guest developer inspection byte was restored and read
back as zero. [Startup debugger snapshot](uikit-startup-v59.lldb).
The entitled app database rebuild returned true; main virtual LCD is 416x496.

Launchd base 0x104a78000 was derived from LR 0x104a9221c and confirmed by
Mach-O magic. At 0x104aa1d0c the extension path was eligible but its UID/GID
was 99:99, mode 0755. A single UID stat-result write at 0x7ac755d0b0 let it
launch; the breakpoint was removed. This remains a diagnostic ownership
override, not an on-disk fix. [Observation](uikit-extension-observe-v59.txt).

## Collation resource fix verified

At runtime 0x1a5183b2c, immediately following
+[UILocalizedIndexedCollation collationWithDictionary:] in currentCollation,
x0 is now non-null (0x0200007672764300), with a non-null input dictionary
x20=0x08000076733eb800. The breakpoint was removed and SpringBoard continued
past the prior "Missing locale identifier in collation dictionary" assertion.
No exception was suppressed. Later startup is still being investigated.


## Next failure: language-identification database

SpringBoard PID 58 subsequently sent itself SIGABRT after 94,907 ms.
The Objective-C exception breakpoint did not fire; stderr instead records:
`Assertion failed: (stat(path, &stat_data) == 0), function _open_and_mmap_for_reading, file LanguageIdentifier.c, line 630.`
[Stderr](uikit-stderr-v59.txt).
Subsequent respawns hit the old extension cast because the ownership diagnostic
was intentionally one-shot. Their cast aborts are not the PID 58 failure.

The matching liblangid contains a stat call at unslid 0x2c254f7f0 in
_langid_env_create and branches to an assertion path on nonzero return.
The guest confirms /usr/share/langid/langid.inv is absent; matching firmware
contains a 9,152,288-byte database. [File and job check](uikit-langid-file-check-v59.txt).
The job list confirms SpringBoard was removed before the targeted restart.
An exact runtime path trace is in progress.


### Exact path and return captured

On restart PID 73, the same extension ownership diagnostic was applied once
at UID slot 0x7ac755b3b0 after verifying the service path. At the stat call
(runtime 0x2e1a3b7f0), x0 points to /usr/share/langid/langid.inv. At the next
instruction 0x2e1a3b7f4, w0=0xffffffff. No return override was used. This
confirms the path behind the assertion. [Restart observation](uikit-langid-observe-v59.txt).

All breakpoints were removed, LLDB detached, and V59 stopped with QMP quit.
The 9,152,288-byte matching langid.inv was copied into the stopped image and
byte-verified. [Image manifest](langid-capture-image-v60.json).

The same image now includes an experimental display-capture probe. It uses
CARenderServerRenderDisplay's 24A437 wrapper at 0x1846dbefc, whose register
shuffling calls 0x1845f6294 and converts its return to bool. It allocates a
416x496 BGRA IOSurface for the diagnostic LCD, reports render success and
pixel-change counts, and writes tightly packed rows to
/private/var/tmp/display-capture.bgra. It removes its prior output before
starting and has a 30-second process alarm. Neither return success nor changed
pixels alone proves SpringBoard content; inspect the captured image.

Compiled with clang -Wall -Wextra -Werror, arm64-apple-ios27.0, linking the
restore libSystem. It carries only the IOSurfaceRootUserClient entitlement
used by the prior layer probe. Its CDHash was added without dropping any old
trust entries (4400 -> 4401). Runtime behavior remains to be tested in V60.
No original Apple binaries or signatures were changed or committed.
