# CoverSheet resource test (24A437, V57)

V56 was confirmed stopped by a missing QMP socket before this boot. The
image contains the byte-verified CoverSheet and CoverSheetKit additions
listed in [the manifest](coversheet-resources-v57.json); the original
ExtensionFoundation directory layout has been restored, without symlinks.

Cache slide: 0x8958000. UserManager PID 38 base: 0x104bc4000. The mobile
NSString at 0x74d10142d0 was checked against UUID
CE7B39AA-4774-5BA4-BD1C-A29AB22DB754 before borrowed argument substitution.
Guest developer inspection byte was restored/read back as zero after the
thread sample. [Startup snapshot](coversheet-startup-v57.lldb).

The entitled application rebuild returned true and the display probe again
reported six displays, main LCD 416x496. Launchd base 0x100954000 was derived
from _xpc_bundle_get_path LR 0x10096e21c and verified by Mach-O magic.
At 0x10097dd0c, the original ExtensionFoundation service path returned helper
value 1, mode 0755, and UID/GID 99:99. Only the UID result at 0x7893538930 was
written to zero, then the breakpoint was removed. This is an explicit
one-shot diagnostic, not repaired on-disk ownership. Extension service PID 59
launched in SpringBoard PID 58's process domain at 00:03:08.978441.
[Observation](coversheet-extension-observe-v57.txt).

## Exact missing asset

CoverSheet resources alone did not fix the exception. The quick-actions view
constructor at unslid 0x224807a20 (runtime 0x22d15fa20) requested the CFString
UICoverSheetButtonLuminanceMap (x2 0x27c175238, bytes at 0x22d1bc0ae).
At the following instruction 0x22d15fa24, the image loader returned x0=0.
Continuing hit objc_exception_throw again. No assertion was suppressed.

The exact name is present in matching UIKitCore.framework/Artwork.bundle/Assets.car.
assetutil identifies a universal 256x1, 8-bit grayscale image, rendition
Button Map 03.png. The guest explicitly reported that catalog did not exist.
[Guest check](coversheet-artwork-check-v57.txt).

SpringBoard removal returned errno 36. All breakpoints were deleted, LLDB
detached, and V57 shut down with QMP quit. The root image was then mounted
and inspected: the entire UIKitCore.framework resource directory was absent.
Artwork.bundle and the parent Info.plist were copied from matching firmware;
all 52 files (4,423,136 bytes) were byte-compared. No firmware is committed.
[Manifest and asset metadata](uikit-artwork-resources-v58.json).
V58 has been started to validate the targeted addition; result pending.

The UART status capture coversheet-live-v57.txt timed out while the guest was
stopped at the image breakpoint, so its silence does not prove a process hang.
No graphical success is claimed.

Artwork executable CDHash 634ecb1504a6a17127d1c0b14184fe5c32af2978 was already present in the 4,400-entry trust cache; no trust entry or original signature changed.

Follow-up: [V58 confirms the artwork fix and records the next collation exception](artwork-runtime-v58.md).
