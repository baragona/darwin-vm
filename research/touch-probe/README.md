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

The V77 preparation passed build, signature, trust-cache and installed-file
verification. Runtime scene presentation and interaction are not yet verified.
The matching `/System/Library/LifecyclePolicy` files are restored in that image:
V74's foreground launch failed before app execution because domain-attribute
policy files were absent. Whether this restoration is sufficient remains to be
measured. See `../evidence/touch-image-v77.json` for local artifact identities.
