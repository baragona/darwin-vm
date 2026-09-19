# V83 short taps and Home resources

V82 is closed after preserving its terminal scheduler panic. V81 was resumed
through LLDB and the existing note/drawing state remains in that guest. A fresh
viewer validated Home frames; a one-move browser swipe did not expose the
Notebook icon. Six frame rejections were observed amid interleaved daemon console
output before a fresh complete frame recovered. App-icon reopening is unverified.

Read-only mounting of the closed V82 image confirmed that
System/Library/PrivateFrameworks/SpringBoardHome.framework is entirely absent,
although SpringBoard.app/SpringBoard.loctable is present. The matching firmware
framework's English table maps REMOVE_APP_SHORTCUT_ITEM_TITLE to Remove App.

V83 clones V82 and restores all45 files from the matching SpringBoardHome
framework, each byte-compared and SHA256-recorded in taps-staging-v83.json.
It installs the strictly cross-built and ad-hoc-signed protocol3 input agent;
the version1 trust cache now has4423 entries and includes its verified CDHash.
The writable mount was detached after verification. No Apple firmware binaries
are included in git.

The agent preallocates a tap's down/up events, refreshes both parent and finger
timestamps immediately before dispatch, and waits40ms between tap dispatches.
A failed release retains held state so R/idle cleanup can retry. Browser tests
exercise real handlers for quick clicks, drags, holds, cancellation, and older
agents. C state tests and all four Python protocol tests pass. This is prepared
for runtime validation, not yet evidence of Calculator arithmetic or reliable
Home-icon reopening. No kernel timing assertions have been bypassed.

V83 reached Bash after31.51host seconds. Kernel UUID and inspection pointer
match the prior build; the inspection byte was read0, temporarily enabled for
UserManager sampling, then restored and read back0 before UI services.
UserManager16 uses base0x10498c000 and shared-cache slide0x14f20000. Launchd
was independently resolved at0x104098000. Both Notebook and Calculator report
LS_IS_INSTALLED=1 after LS_REBUILD_RESULT=1 before SpringBoard starts.
SpringBoard49 reached UIApplicationMain. A stopping, read-only observer at
0xfffffe002abf6e18 will preserve scheduler operands if the prior clock failure
recurs; no scheduler code is bypassed. The protocol3 viewer is starting.

The protocol3 agent announced LIVE_INPUT_READY3 and produced a validated lock
screen (taps-lock-screen-v83.png). An actual browser unlock gesture was attempted
but the bridge no longer sent input. The browser queue was empty and the UI
reported Connected while QMP confirmed the guest still running. A read-only host
stack sample found only the UART reader and HTTP threads; the writer had exited.
Source inspection identifies the failure: ack.wait(120) permanently stopped the
writer during a slow frame, then a late ack reset the displayed status to
Connected. This is a host transport failure, not proof that iOS ignored input.

The host now preserves its single outstanding command across acknowledgement
observation windows. Late responses resume the same writer without retransmitting
the command; explicit shutdown or UART closure still terminates it. A regression
test simulates two expired windows, a late ack, and a queued release, proving
that the original command is sent once and the release follows. All five Python
protocol tests and browser handler tests pass. Live verification is next.
