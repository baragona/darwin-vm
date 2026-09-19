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

V77 passed build, signature, trust-cache and installed-file verification. A normal
LaunchServices request reached UIKit scene activation after restoring lifecycle
policies, provisioning missing kernel persona 1003, and temporarily extending the
scene watchdog. A validated 832x1808 frame contains `Taps: 0` and the button
background; the button title is missing and lock-screen controls remain overlaid.
A stationary touch reached the guest input monitor, but the fresh frame stayed
unchanged and no app tap callback was observed. Interaction and home/reopen are
still unverified. See [runtime findings](../evidence/touch-runtime-v77.md) and
[image identities](../evidence/touch-image-v77.json).


A follow-up attention-sensing bypass let SpringBoard reach its event-queue drain
and a fresh app reach scene activation. The fresh capture still showed the lock
screen covering the app. An upward swipe completed at the input monitor, followed
by a BackBoard abort/restart of undetermined cause. This does not yet establish
button interaction; see the runtime findings for the diagnostic scope and evidence.
