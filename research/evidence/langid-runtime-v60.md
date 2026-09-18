# Language database and display capture (V60, 24A437)

Cache slide 0x9364000. UserManager PID 38 base 0x10094c000; verified mobile
NSString 0x76fac10280. Guest developer inspection byte restored/read back zero.
[Startup debugger snapshot](langid-startup-v60.lldb).
Database rebuild returned true and main LCD is 416x496.

Launchd base 0x10480c000 was derived from LR 0x10482621c and verified by
Mach-O magic. The original extension path passed the framework check at
0x104835d0c but had UID/GID 99:99 and mode 0755. One stat UID write at
0x7ac48fca70 allowed launch; the breakpoint was removed. This remains a
diagnostic workaround for image ownership.

## Capture probe baseline

[Actual guest run](display-capture-baseline-v60.txt) reports main LCD, successful
IOSurface allocation, CARenderServerRenderDisplay result 1, successful file
write/unlock, and process exit 0. However **zero active pixels changed** from
the 0xa5 sentinel, zero were opaque, and zero had changed color. Thus no UI
was captured despite API success. The 825,344-byte BGRA file is unchanged
sentinel data, not a screenshot of SpringBoard. This run occurred during
SpringBoard startup; a later retry is needed to assess startup timing.

The probe signature verified with codesign --verify --strict. Its only IOKit
entitlement is IOSurfaceRootUserClient; no additional capture entitlement or
Developer Mode override was needed for this baseline API call to return.
At the previously failing stat return (0x2cb8b37f4), w0 is now zero and
x20 still names /usr/share/langid/langid.inv. No return override was applied.
The breakpoint was removed and startup continued. This verifies the missing
file lookup is corrected; later initialization remains under observation.


## First layer-commit failure

After the language stat succeeds, objc_exception_throw catches
CASecureIndicatorLayerInvalidName, reason "Invalid indicator name Camera".
[Symbolicated stack](langid-next-exception-v60.json) identifies
CASecureIndicatorLayer _copyRenderLayer inside CA::Context::commit_transaction,
reached from UIApplication's first-commit block. This is later than the
previous resource failures but still not a completed first render commit.

QuartzCore's indicator_id_from_name checks a non-null optional function at
runtime pointer slot 0x1f20952a8, value 0x2a7ea5c38, which resolves to
SILManagerIndicatorTypeIDFromName. If that function is absent, existing code
maps Camera -> 0, Microphone -> 1, MicrophoneAccessibility -> 2, FaceID -> 3.
The four fallback strings were read from the guest's actual shared cache.

The guest confirms SILManager.framework/cam_mic.plist is absent.
[File check and assertion](indicator-files-v60.txt). Matching firmware's
manifest explicitly defines Camera type 0, Microphone type 1, and
MicrophoneAccessibility type 2. The resource hypothesis is therefore concrete,
but the normal SILManager manifest load has not yet been repaired or retested.
A scoped runtime test selected that existing fallback: at runtime
0x18d8d3e68 (unslid 0x18456fe68), set x8 to zero before the optional-function
branch. The built-in Camera lookup returned w0=0 at runtime 0x18d8d3ef4.
This was a register override on the lookup breakpoint, not a durable fix.
SpringBoard PID 61 survived for several guest minutes with this override.
The exception breakpoint did not catch another Objective-C exception.

## Capture and main-thread follow-up

[Capture after the fallback](display-capture-indicator-fallback-v60.txt)
returned API success and wrote all 825,344 bytes. All 206,336 active pixels
changed from the sentinel, but none had nonzero RGB and none had alpha 255.
Thus the capture contains no visible UI; changed bytes alone are not success.
Alpha values were not individually exported, so this does not establish that
every alpha value was zero.

[Successful thread sample](indicator-springboard-threads-retry-v60.txt)
reports task access success, 78 threads, and successful resume. The earlier
attempt returned task error 5 because enabling the kernel inspection byte
while stopped in userspace failed. Retrying in kernel context allowed writing
and reading back 1 at 0xfffffe0017088db4. After sampling, this byte was restored
to 0 and read back in kernel context.

[Symbolicated main thread](indicator-main-thread-v60.json) shows GSEventRunModal
and CFRunLoopRun below a delayed timer callback. That callback enters
SBTelephonyManager's telephony-daemon restart handling, updates the idle timer,
asks isInEmergencyCallbackMode, and waits synchronously through CoreTelephony
and XPC in mach_msg2_trap. Repeated launchd logs report missing
com.apple.commcenter.xpc. This establishes that SpringBoard reached its event
loop, with the sampled callback waiting on telephony. A single sample does
not establish permanent deadlock or prove this is the only cause of black
capture. Missing audio services also appear in the log.

## Stopped image prepared for V61

All six remaining debugger breakpoints were removed, LLDB detached, and
QEMU exited successfully before mounting the image. Restored all seven
original SILManager.framework resource files (19,426,752 bytes); each was
absent, and each copied file was compared byte-for-byte with matching firmware.
[Hashes and paths](secure-indicator-resources-v61.json). The image was detached
successfully. No trust-cache update or executable patch was needed for these
resources. Normal SILManager initialization remains untested until V61.

The guest is stopped. Next boot should test the real indicator lookup without
the x8 fallback, then investigate telephony waits and repeat display capture.
Usable UI, display presentation, and interactive input remain unachieved.
