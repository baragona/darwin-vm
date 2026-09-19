# Persistent guest input prototype

`live-input-agent.c` maintains one HID virtual touch service while accepting
newline-delimited input on stdin. `live-display.c` captures the real QuartzCore display into a reusable IOSurface
and streams compressed BGRA frames. `live-view.py` displays those frames in a
local browser and forwards pointer and physical keyboard events.

Protocol (normalized coordinates, case-sensitive commands):

- `D X Y`: finger down; rejected if a finger is already down.
- `M X Y`: move the active finger; rejected without a preceding down.
- `U X Y`: release the active finger.
- `T X Y`: short tap (protocol3), with both events allocated before pressing and
  a 40ms guest delay before release. Rejected while a finger is already down.
- `S X0 Y0 X1 Y1`: straight swipe (protocol4). All14 touch events are allocated
  before pressing, then dispatched40ms apart. Rejected while a finger is down.
- `K USAGE DOWN`: keyboard page7 usage4..231, with DOWN0 or1.
- `H`: consumer Menu/Home key down followed by release.
- `R`: release all tracked touch and keyboard state.
- `F`: capture and stream a lossless frame or changed rectangle (protocol2).
- `G`: force a full frame to recover missing or damaged history.
- `P`: acknowledge a ping.
- `Q`: release input and exit.

The agent emits `LIVE_INPUT_READY 4` (the host also accepts versions1–3), then `LIVE_INPUT_RESULT 0|1` for each
line. It rejects out-of-range coordinates, malformed or oversized commands,
and invalid touch transitions. It pumps the run loop while waiting for input.
EOF, SIGTERM, SIGINT, and five seconds without successful input commands (frame requests and pings do not count) trigger release attempts.
A failed touch dispatch does not advance the tracked touch state. Keyboard
API return is void, so acknowledgement means construction and dispatch, not
confirmed delivery. Host disconnect does not necessarily become guest stdin
EOF over UART; the idle release is necessary for that transport.

With protocol3, the browser defers a stationary press for up to200ms. Releasing
within that window sends `T`; movement of at least3 CSS pixels flushes the
original down and continues a normal drag. Holding beyond200ms also flushes the
down. Cancellation discards a pending tap and sends release. Older agents retain
the original D/U path. This removes UART acknowledgement waits from a quick tap's
press/release interval; it does not bound delays inside guest HID dispatch or
fix capture latency. `node research/live-pointer-test.js` exercises the actual
browser handlers for taps, drags, holds, cancellation, and legacy guests.

With protocol4, the mouse wheel sends a guest-timed swipe. Vertical wheel motion
scrolls; horizontal wheel motion or Shift+wheel changes Home pages. Gestures are
throttled to one per650ms, and wheel input is ignored during a pointer drag.
The guest preallocates the whole path, avoiding UART waits and allocation between
touch events. The path is still real guest HID input; a dispatch acknowledgement
does not prove the target view scrolled. V84 is staged for runtime testing.

Runtime update: V81 verified browser keyboard input, saving, and drawing using
protocol2. V82 restored CoreGlyphs and rendered Apple Calculator controls, then
later suffered a scheduler timestamp panic; arithmetic is not yet verified.
V83 stages protocol3 plus the missing SpringBoardHome framework resources.
Detailed runtime evidence
is in `evidence/live-drawing-runtime-v81.md` and `evidence/glyphs-runtime-v82.md`.

Build and sign like the diagnostic helper, substituting live-input-agent.c:

```sh
xcrun clang -O2 -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib \
  research/live-input-agent.c research/live-display.c research/virtual-touch-service.c \
  /path/to/libSystem.B.dylib -o /tmp/live-input-agent
codesign -s - --entitlements research/live-agent-entitlements.plist /tmp/live-input-agent
codesign --verify --strict /tmp/live-input-agent
```

Install in a new guest image with its CDHash in that image's trust cache.
Strict cross-build and signature verification pass. Host-only stub tests verify
invalid commands dispatch nothing, tap events are allocated before dispatch,
failed touch release preserves state, and reset releases tracked keys and touch.
No host HID API is used by those tests. See the runtime status above for the
separate guest verification.

Run host-only state tests with `clang -Wall -Wextra -Werror research/live-input-test.c -o /tmp/live-input-test && /tmp/live-input-test`.

## Local browser prototype

With exclusive ownership of the guest UART and an idle guest shell:

```sh
python3 research/live-view.py --serial /tmp/a19-ui-v80-serial.sock \
  --transcript /tmp/live-view-v80.txt --start
```

Open the printed localhost URL. Click the display to focus keyboard input;
Home sends a real consumer Menu event. The key mapping uses physical US key
positions. The page reports age of the last validated frame. Frame IDs,
compressed lengths, CRC32, decompressed lengths, and stream termination must
all match before pixels appear. Corrupt frames are discarded, not repaired.

UART is shared by display and input: capture blocks guest input while the frame
is written. The host avoids starting captures during active touches or held
keys, coalesces pending moves, and bounds queues. This is an initial transport;
a V83 observation measured roughly700 UART output bytes per host second. The
writer keeps waiting across120-second acknowledgement observation windows; it
does not resend an outstanding command or exit solely because that window ends.
Closing the
viewer disconnects UART; guest idle cleanup releases input after five seconds
of guest execution, but the agent remains running. Do not use `--start` again
until it has been stopped and the shell recovered.

Host tests: `python3 research/live-view-test.py` checks pixel preservation,
corruption/truncation rejection, recovery, and input queue ordering. These
checks do not establish guest rendering or keyboard delivery; the recorded
V81 runtime provides separate evidence for those behaviors.

## Changed rectangles (protocol2 and later)

The first capture is a full `LIVE_FRAME_BEGIN` frame. Later captures compare
actual BGRA pixels against the previous capture and send the smallest enclosing
changed rectangle as `LIVE_PATCH_BEGIN seq parent canvasWidth canvasHeight x y
width height rawBytes packedBytes crc32`. Data and end records retain the
original format. Data chunks are now96 bytes to shorten exposure to interleaved
console logs. Updates remain lossless at the original resolution.

The receiver applies a patch only after verifying its base sequence, rectangle
bounds, compressed CRC, exact decompressed length, and complete stream. It
retains the last good image on failure and requests `G` for recovery. An unchanged
capture emits `LIVE_FRAME_SAME seq`; the host refreshes its frame age only if it
already holds that exact valid frame. No pixel values are guessed or repaired.

`frame-damage-test.c` tests bounds and padded rows. The Python viewer tests also
cover exact patch application, rejected stale bases, unchanged frames, truncated
patches, and full-frame recovery. V81 verified full and changed-rectangle frames
in the guest. UART contention and capture latency remain limitations.
