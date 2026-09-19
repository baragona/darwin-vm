# Touch Probe

A minimal UIKit app for the A19/iOS 27 guest. It uses a real UIApplication,
UIWindowScene, view controller, label, and button. Each touch-up-inside event
increments a visible counter and logs `Taps: N`. It does not bypass the normal
application lifecycle or draw directly into the emulator framebuffer.

Build using the matching firmware's locally extracted libSystem stub:

```sh
python3 research/touch-probe/build.py \
  --libsystem /tmp/a19-restore/usr/lib/libSystem.B.dylib \
  --output /tmp/touch-probe-build \
  --trustcache firmware/iphone-17-ui-probe/geometry-v74.tc
```

The output directory must be new. The script builds with warnings as errors,
signs and verifies the bundle, and optionally extends a version-1 trust cache.
Install `TouchProbe.app` under `/Applications` in a separate guest-image clone,
and boot that clone with the generated trust cache. No Apple headers, framework
binaries, or firmware policy contents are included here. The C implementation
uses the arm64 Objective-C ABI and loads the guest's own UIKit at runtime.

This app intentionally supports one portrait scene. It needs no camera,
network, account, media, or Settings-specific services. Its success criteria are:

1. LaunchServices registers `org.baragona.TouchProbe`.
2. A normal foreground launch reaches its scene callback.
3. A display capture contains `Taps: 0` and `Tap me`.
4. Injected touch input produces `Taps: 1` both in the app log and in a fresh
   display capture. Callback delivery alone is insufficient.
5. Returning home and reopening the app works.

Diagnostic messages go to stdout and, when writable, to
`/var/mobile/Library/Logs/TouchProbe.log`. `SCENE_VISIBLE_REQUESTED` only records
a UIKit request; it does not prove that the frame was presented.

V77 now has one verified button interaction: virtual touchscreen input reached
the real UIKit callback, which logged `Taps: 1`, and a fresh 832x1808 display
capture visibly shows `Taps: 1`. The preceding upward swipe reached UIKit and
CoverSheet and was followed by a visible app without the lock-screen controls.
See the [successful capture](../evidence/touch-first-success-v77.png) and
[recovery and input findings](../evidence/touch-backboard-recovery-v77.md).

This remains a diagnostic guest. Launch requires restored lifecycle policies,
a provisioned kernel persona, and temporary bootstrap, attention-sensing, and
scene-watchdog workarounds. The stationary touch currently uses a debugger
coordinate conversion in the input constructor; app state and pixels are not
modified. The button title is still missing, and home/reopen, normal launch
timing, continuous display, and host pointer integration remain unverified.
Earlier [runtime findings](../evidence/touch-runtime-v77.md) and
[image identities](../evidence/touch-image-v77.json) preserve the failed runs.
