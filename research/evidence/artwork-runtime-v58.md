# UIKit artwork runtime test (V58)

The original UIKitCore Artwork.bundle and framework Info.plist were added
from matching 24A437 firmware. [Manifest](uikit-artwork-resources-v58.json).
No original signatures or trust entries were changed.

Cache slide 0xfc7c000. UserManager PID 38, base 0x100fd4000; mobile NSString
0x73d4c20230 was verified before borrowed argument substitution. Developer
inspection byte restored/read back zero. [Debugger snapshot](artwork-startup-v58.lldb).
Entitled database rebuild returned true, main LCD again 416x496.

Launchd base 0x102fa8000 (derived from LR 0x102fc221c, verified Mach-O magic).
At 0x102fd1d0c the original extension path passed the framework helper but
had UID/GID 99:99, mode 0755. A single temporary stat UID write at
0x7490890930 allowed launch, then the breakpoint was removed. This does
not correct image ownership. [Launch observation](artwork-extension-observe-v58.txt).

Targeted image-return breakpoint: 0x234483a24 (unslid 0x224807a24).
Exception breakpoint: 0x19008c328 (objc_exception_throw).
The targeted image-loader return now has x0=0x0d00007d1d672760, a non-null
object, whereas V57 returned nil at the same instruction. The image-return
breakpoint was deleted and startup continued with the exception breakpoint
still active. This verifies the missing asset can now load; it does not yet
prove a usable UI.


## Result: image assertion cleared, App Library collation fails

After continuing from the non-null image return, the next captured exception
has reason "Missing locale identifier in collation dictionary". Its stack
starts with UILocalizedIndexedCollation initWithDictionary/currentCollation,
then SBHLocalizedIndexedCollationStrategy, SBHIconLibraryTableViewController,
and SBHomeScreenController's library setup. [Symbolicated stack](artwork-next-exception-v58.json).
The earlier luminance assertion was not suppressed: restoring the artwork
allowed execution to advance to this different App Library initialization.

The guest confirms UIKitCore.framework/en.lproj/UITableViewLocalizedSectionIndex.plist
is absent. Matching firmware provides UICollationKey=en@collation=dictionary
and the index/section arrays. The stderr capture confirms the new exception.
[File check and stderr](artwork-collation-check-v58.txt).

## Cleanup and prepared next test

SpringBoard removal returned errno 36 during teardown. All debugger breakpoints
were removed, LLDB detached, and V58 was stopped with QMP quit. The image was
then mounted and the remaining UIKitCore resource tree copied: 84 new files,
2,468,172 bytes. The previous 52 files were verified unchanged; every new file
was compared byte-for-byte against matching firmware. This completes all 136
files (6,891,308 bytes) from that framework resource tree.
[Addition manifest](uikit-resources-v59.json).

V59 has not booted: the collation fix remains unverified. The image is detached,
no VM is running, and no debugger is attached. Guest developer override was
restored to zero before startup. No host security settings changed.

The next boot still needs the documented persona/display/credential diagnostics
and the one-shot extension ownership diagnostic. Permanent ownership repair,
further startup dependencies, SpringBoard pixels, and interactive input remain
outstanding. No usable graphical emulator is claimed.
