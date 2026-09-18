# CGImage-backed CALayer renders correctly in the local software compositor

The Settings screenshot alone cannot distinguish missing artwork from broken
bitmap compositing. `software-render-probe --bitmap` isolates the latter: it
creates a 64x64 RGBA CGImage from four opaque color quadrants, assigns it to
CALayer.contents, commits a local CAContext, and renders through the existing
24A437 software-renderer path. It does not use IconServices, LaunchServices,
image-file decoding, IOSurface, or remote display-server transport.

## Verified guest result

A separate V75 guest booted from a clone of the V74 image with the rebuilt,
ad-hoc signed probe and one additional trustcache entry (4415 total).
The original V74 UI guest was preserved. See bitmap-image-v75.json for identity.
The probe completed its terminal marker and exited zero:

```
SW_BITMAP_IMAGE_PRESENT=1
SW_CHANGED_BYTES=16384 GUARD_CHANGED_BYTES=0
SW_BITMAP_RED=1024 GREEN=1024 BLUE=1024 YELLOW=1024 OTHER=0 PASS=1
SW_OUTPUT_BYTES=16384 CLOSE_RESULT=0
SW_PROBE_END
BITMAP_EXIT=0
```

The 16,384-byte BGRA output was compressed and exported through the UART.
The dimension-aware decoder validated it at64x64. Its SHA256 is
883e6c79096ca7ebfa73b545055d2d49ca7a178cb391ed04bedf2eba06f33c8c.
An independently generated expected BGRA quadrant buffer has exactly this hash,
verifying placement and orientation as well as color counts. The decoded PNG
was also visually inspected: red/green above blue/yellow.

Evidence: software-bitmap-v75.txt, software-bitmap-export-v75.txt,
software-bitmap-v75.json, software-bitmap-v75.png, bitmap-boot-v75.txt.
The generic boot script's thread-probe diagnostics were denied because developer
mode was disabled; the bitmap probe requires no such entitlement and completed.

## Scope and next test

This rules out a blanket inability of the guest's local software compositor to
render CGImage-backed layer contents. It does not prove remote image transport,
IOSurface-backed textures, PNG/CgBI or asset-catalog decoding, IconServices
output, or SpringBoard's final display composition. It does not fix the icon
scale or produce a working Settings app.

Next isolate actual Settings image decoding and/or a bitmap-backed remote layer.
Keep the quarter-scale view investigation separate until the visible view's
current transform and caller are established.

## Reproduce

Build with the same flags as the existing software-render probe:

```
xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib research/software-render-probe.c \
  /tmp/a19-restore/usr/lib/libSystem.B.dylib -o /tmp/a19-software-bitmap-v75
codesign -s - /tmp/a19-software-bitmap-v75
codesign --verify --strict /tmp/a19-software-bitmap-v75
```

Install as /bin/software-render-probe in a cloned guest image and include its
actual CDHash in that image's trustcache. Run `/bin/software-render-probe --bitmap`.
The output is /private/var/tmp/software-bitmap.raw. The probe's per-color check
requires exactly1024 pixels of each color and no other pixels; guard corruption
or a color mismatch returns nonzero. Existing modes remain available.

After export, V75 reported running through QMP and accepted the intentional quit
request. V74 independently reported running afterward. The isolated test VM was
stopped to release its memory; its bootable image remains available locally.
