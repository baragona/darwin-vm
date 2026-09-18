# Original secure-indicator resources (V61, 24A437)

Booted the stopped image containing the seven byte-verified SILManager files.
Cache slide 0x2bcc000; UserManager PID 38 base 0x102948000. An initial
thread sample was too early (dyld not initialized); the second succeeded.
The mobile NSString 0x7ba9020230 was checked against the exact mobile UUID.
The developer inspection byte was restored and read back zero before services.
[Startup diagnostic snapshot](indicator-startup-v61.lldb).

System app rebuild returned true; virtual main LCD is 416x496. RunningBoard
PID 58, SpringBoard PID 60, backboard PID 52. Launchd base 0x100670000 was
derived from LR 0x10068a21c and verified by Mach-O magic. At 0x100699d0c,
the original extension path passed its framework check (x0=1), mode 0755,
UID/GID 99:99. A one-shot stat UID write at 0x78028f4e30 allowed launch,
and that breakpoint was deleted. No secure-indicator fallback was installed.

Observation breakpoint 0x18713be78 is immediately after the optional
SILManager name lookup call in QuartzCore (unslid 0x18456fe78). The real call returned w0=0xffffffff for x19=0x1ebbe50c0, the original
Camera CFString (six bytes at 0x1873e037f). No indicator-result or branch
override was applied. The subsequent Objective-C exception reason at
0x76b91a9ed1 is "Invalid indicator name Camera". Thus copying the ordinary
SILManager framework resources alone does **not** fix the manifest lookup.

## Actual resource path

Static code for resolvedManifestDirectory (unslid 0x29eb7826c) checks the
ExclaveOS SILManagerAssets path, then falls back to SILManagerComponent's
secureindicatorassets directory. Both live under the ExclaveOS cryptex.
The ordinary /System/Library/PrivateFrameworks/SILManager.framework directory
is not the selected location in this execution.

Allowed SpringBoard to exit and respawn as PID 64; the original extension
path again passed the framework check and needed one stat UID override at
0x78028f2eb0. That breakpoint was removed. At manifest initializer runtime
0x2a17311ec (unslid 0x29eb651ec), immediately after getManifestDirectory,
Swift string return registers were:

```
x0 = 0xd000000000000085
x1 = 0x80000002a176dd60
```

Reading its bytes at (x1 & 0x7fffffffffffffff)+0x20 confirms the 133-byte path:

```
/private/preboot/Cryptexes/ExclaveOS/System/ExclaveKit/System/Library/Frameworks/SILManagerComponent.framework/secureindicatorassets/
```

The preferred path in the same code is:

```
/private/preboot/Cryptexes/ExclaveOS/System/ExclaveKit/System/Library/PrivateFrameworks/SILManagerAssets.framework/
```

All six remaining debugger breakpoints were removed, LLDB detached, and QEMU
exited successfully before image editing. The restored developer byte remains
zero; the second startup did not need it enabled.

## Matching ExclaveOS assets prepared for V62

The exact 24A437 BuildManifest identifies Ap,ExclaveOS as
043-69885-754.dmg.aea. Extracted that component from the original Apple IPSW,
decrypted it with ipsw fw aea, and mounted it read-only. Both directories
exist in that image. Restored all 17 resource files at their original paths
under the guest's /private/preboot/Cryptexes/ExclaveOS prefix and compared each
copy byte-for-byte. The preferred asset directory includes
cam_mic_constraints_V53_V57~iphone.plist, matching this guest's V57 board.
[Resource hashes and paths](exclave-indicator-assets-v62.json).
No ExclaveOS executable was installed or launched; this supplies resource data
for the iOS SILManager client, not ExclaveOS emulation.

## Telephony experiment prepared separately

[Guest service check](indicator-services-v61.txt) confirms CommCenter is absent.
The V60 main-thread sample waits in its synchronous emergency-callback query.
Staged the matching CoreTelephony framework, including its original CommCenter
executable, and the original noncellular launch configuration with KeepAlive
disabled, its device-tree exclusion removed for manual testing, and output
redirected to /var/tmp. The daemon's original CDHash is already present in
the 4,401-entry trust cache; no resigning or trust-cache change was needed.
[Image manifest](commcenter-image-v62.json),
[diagnostic job](../commcenter-noncellular-probe.plist).

This job is not automatically launched. After validating the corrected
indicator lookup, explicitly submit /launchjobs/com.apple.CommCenter.probe.plist
and inspect its output, process lifetime, and SpringBoard's main thread before
attributing any capture change to telephony. It may need additional services
or state directories; working noncellular behavior is not established.

The guest is stopped with V62 image preparation complete. Next boot must
validate indicator lookup without its fallback and repeat actual display
capture. No usable UI or interactive input has been demonstrated.
