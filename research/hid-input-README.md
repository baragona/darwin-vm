# Guest input probe

This diagnostic uses the guest's IOKit and HID.framework. Input-monitor
callbacks only establish that events entered the input system; verify the
intended UI response separately.

```sh
/bin/hid-input-probe --virtual-swipe
/bin/hid-input-probe --virtual-tap 0.5 0.55
```

The tap takes normalized X/Y coordinates in [0,1], measured from the display's
top-left corner. It registers the same virtual touchscreen as the swipe,
discovers the current display association, and sends a stationary contact with
an explicit release. Both hand and finger events carry the requested position.
The 14-frame cadence matches the V77 successful interaction experiment. All
events are allocated before any input is dispatched. NaN, infinity, trailing
junk, missing arguments, and out-of-range values fail with exit2 before loading
input frameworks. The helper retains its existing bounded runtime alarm.

The original no-argument monitor, `--home`, `--swipe`, and `--virtual-swipe`
modes remain available. `--home` sends a consumer Menu key; its acceptance by a
particular guest UI must be verified independently.

Build with the matching firmware's locally extracted libSystem stub:

```sh
xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib \
  research/hid-input-probe.c research/virtual-touch-service.c \
  /path/to/libSystem.B.dylib -o /tmp/hid-input-probe
codesign -s - --entitlements research/hid-input-entitlements.plist /tmp/hid-input-probe
codesign --verify --strict /tmp/hid-input-probe
```

Install the signed helper as `/bin/hid-input-probe` in a separate guest-image
clone and include its CDHash in that boot's trust cache. No firmware binaries
are distributed here.

V78 verified this native command end to end: 14 dispatches, release, a real
UIKit callback logging `Taps: 1`, and a fresh frame showing that counter.
See [runtime evidence](evidence/native-tap-runtime-v78.md). This diagnostic boot
still requires graphics and startup workarounds. Its fixed helper alarm was
too short under load; testing used `(trap '' ALRM; /bin/hid-input-probe
--virtual-tap 0.5 0.55)` with a separate observation deadline. It is not yet
a persistent host mouse-input service.

`--virtual-drag X0 Y0 X1 Y1` adds a straight drag with normalized endpoints,
using the same 14-frame allocation, dispatch, and release sequence. Its source
passes strict iOS compilation and host parser checks; it is not yet installed
or verified in V78. The existing fixed upward swipe starts at Y=0.96, which
left Touch Probe visible in the first Home-navigation test. That observation
does not prove whether the starting position, duration, or another gesture
condition caused the failed navigation.
