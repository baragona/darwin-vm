# Persistent guest input prototype

`live-input-agent.c` maintains one HID virtual touch service while accepting
newline-delimited input on stdin. `live-display.c` captures the real QuartzCore display into a reusable IOSurface
and streams compressed BGRA frames. `live-view.py` displays those frames in a
local browser and forwards pointer and physical keyboard events.

Protocol (normalized coordinates, case-sensitive commands):

- `D X Y`: finger down; rejected if a finger is already down.
- `M X Y`: move the active finger; rejected without a preceding down.
- `U X Y`: release the active finger.
- `K USAGE DOWN`: keyboard page7 usage4..231, with DOWN0 or1.
- `H`: consumer Menu/Home key down followed by release.
- `R`: release all tracked touch and keyboard state.
- `F`: capture and stream a lossless frame.
- `P`: acknowledge a ping.
- `Q`: release input and exit.

The agent emits `LIVE_INPUT_READY 1`, then `LIVE_INPUT_RESULT 0|1` for each
line. It rejects out-of-range coordinates, malformed or oversized commands,
and invalid touch transitions. It pumps the run loop while waiting for input.
EOF, SIGTERM, SIGINT, and five seconds without successful input commands (frame requests and pings do not count) trigger release attempts.
A failed touch dispatch does not advance the tracked touch state. Keyboard
API return is void, so acknowledgement means construction and dispatch, not
confirmed delivery. Host disconnect does not necessarily become guest stdin
EOF over UART; the idle release is necessary for that transport.

Build and sign like the diagnostic helper, substituting live-input-agent.c:

```sh
xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib \
  research/live-input-agent.c research/live-display.c research/virtual-touch-service.c \
  /path/to/libSystem.B.dylib -o /tmp/live-input-agent
codesign -s - --entitlements research/live-agent-entitlements.plist /tmp/live-input-agent
codesign --verify --strict /tmp/live-input-agent
```

Install in a new guest image with its CDHash in that image's trust cache.
Current status: strict cross-build and signature verification pass. Host-only
stub tests verify invalid commands dispatch nothing, failed touch release
preserves state, and reset releases tracked keys and touch. No host HID API is
used by those tests. The persistent endpoint itself is not yet guest-tested;
only the separate one-shot touch helper's taps/drags/Home have runtime proof.

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
frame rate and input latency still need measurement in the guest. Closing the
viewer disconnects UART; guest idle cleanup releases input after five seconds
of guest execution, but the agent remains running. Do not use `--start` again
until it has been stopped and the shell recovered.

Host tests: `python3 research/live-view-test.py` checks pixel preservation,
corruption/truncation rejection, recovery, and input queue ordering. These
checks do not establish guest rendering or keyboard delivery. The combined
agent and short-tap helper are staged in V80; runtime verification is pending.
