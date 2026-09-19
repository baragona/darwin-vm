# Persistent guest input prototype

`live-input-agent.c` maintains one HID virtual touch service while accepting
newline-delimited input on stdin. This is the guest endpoint for a future live
host window. It does not yet include display streaming or a host frontend.

Protocol (normalized coordinates, case-sensitive commands):

- `D X Y`: finger down; rejected if a finger is already down.
- `M X Y`: move the active finger; rejected without a preceding down.
- `U X Y`: release the active finger.
- `K USAGE DOWN`: keyboard page7 usage4..231, with DOWN0 or1.
- `H`: consumer Menu/Home key down followed by release.
- `R`: release all tracked touch and keyboard state.

The agent emits `LIVE_INPUT_READY 1`, then `LIVE_INPUT_RESULT 0|1` for each
line. It rejects out-of-range coordinates, malformed or oversized commands,
and invalid touch transitions. It pumps the run loop while waiting for input.
EOF, SIGTERM, SIGINT, and five seconds without input trigger release attempts.
A failed touch dispatch does not advance the tracked touch state. Keyboard
API return is void, so acknowledgement means construction and dispatch, not
confirmed delivery. Host disconnect does not necessarily become guest stdin
EOF over UART; the idle release is necessary for that transport.

Build and sign like the diagnostic helper, substituting live-input-agent.c:

```sh
xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib \
  research/live-input-agent.c research/virtual-touch-service.c \
  /path/to/libSystem.B.dylib -o /tmp/live-input-agent
codesign -s - --entitlements research/hid-input-entitlements.plist /tmp/live-input-agent
codesign --verify --strict /tmp/live-input-agent
```

Install in a new guest image with its CDHash in that image's trust cache.
Current status: strict cross-build and signature verification pass. Host-only
stub tests verify invalid commands dispatch nothing, failed touch release
preserves state, and reset releases tracked keys and touch. No host HID API is
used by those tests. The persistent endpoint itself is not yet guest-tested;
only the separate one-shot touch helper's taps/drags/Home have runtime proof.

Run host-only state tests with `clang -Wall -Wextra -Werror research/live-input-test.c -o /tmp/live-input-test && /tmp/live-input-test`.
