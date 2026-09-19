# V77 compositor recovery and input follow-up

After BackBoard51's unexplained SIGABRT, BackBoard163 started without the
process-local virtual-display setup. A fresh capture reported
DISPLAY_CAPTURE_NO_MAIN_DISPLAY. Reapplying the established virtual-display
flag, LCD-name, and 832x1808 constructor hooks before restarting as BackBoard167
reached each hook once. These are same-boot addresses, not a reusable recipe
for a fresh boot; the cache slide remains 0x2920000.

An early display-probe169 query aborted in QuartzCore query_displays (runtime
LR 0x186e0af10). The captured stack reaches ensure_displays and CADisplay displays.
This is a separate diagnostic-process abort and does not explain BackBoard51's
prior SIGABRT. BackBoard167 remained running. Its sampled main thread and render
threads were in run loops or Mach message waits; an active worker was formatting
an os_log message. The sample does not demonstrate a persistent deadlock.

The later display query completed with DISPLAY_EXIT=0, LCD ID1, main display
present, and mode832x1808. The follow-up capture exported a full-size buffer but
reported zero changed/colored/opaque pixels by the guest prefill-aware check.
The independent decoder found no opaque pixels and SHA-256
d1d3d447139ed59fcead2349ae54ab3f824e6d99792b9c3d3d2da4993301ec85;
its simple nonzero-RGB metric counts the 0xa5 prefill as colored, which is not
proof of rendering. Therefore restoring the compositor
alone did not demonstrate restored visible UI. SpringBoard is being restarted
with the existing exact-path extension bundle stat-UID workaround in place.

The thread probe completed with RESUME_RESULT=0. Restoring the temporary guest
inspection byte from a userspace debugger stop failed without changing it;
a one-shot kernel breakpoint then restored it and read back0 before resuming.
No host security settings changed.


SpringBoard176 restarted successfully; extensionkitservice's exact-path stat
result again had mode040755 and UID99. The one-shot callback wrote four zero
UID bytes at 0x77b95ec6b0 and disabled itself. Its first capture attempt still
reported display off and hit the capture helper's 30-second alarm. Subsequently
the attention bypass callbacks began executing in the new SpringBoard.

A bounded read-only observer is armed for UIApplication sendEvent:, UIWindow
sendEvent:, CoverSheet's dismiss-gesture-began method, and SBLockScreenManager's
unlockUIFromSource:withOptions:. Each method disables its observer at 96 hits;
the callback does not change events, app state, or gesture outcomes. This avoids
the earlier high-volume drain logging while testing actual UIKit delivery.


The first open helper exited142 on its 30-second alarm, but the same request
later spawned Touch Probe: its new Mach-O base is0x102994000, verified with
0xfeedfacf at UIApplicationMain entry. The observer moved to the new log_event
address0x102998584. This is another concrete example of a helper timeout not
cancelling the underlying launch request; no duplicate open was sent.


The fresh app (PID180) reached DID_FINISH_LAUNCHING, SCENE_CONNECT_BEGIN,
SCENE_VISIBLE_REQUESTED, and SCENE_ACTIVE. A serial sandbox denial explicitly
confirms why the old /var/mobile/Library/Logs/TouchProbe.log path is unwritable.
The read-only log_event observer continues to capture its messages.

The observed swipe completed14 virtual dispatches, all returning1, with14
matching monitor callbacks including release and SWIPE_EXIT=0. Unlike the older
runs, the UIKit observers recorded15 UIApplication sendEvent: entries,
15 UIWindow sendEvent: entries, and one CoverSheet dismiss-gesture-began entry.
The extra method entry must not be equated with an extra injected frame.
No unlockUIFromSource:withOptions: entry was recorded. This proves delivery
through UIKit and entry to the dismissal gesture; it does not alone prove that
the lock screen dismissed or the app received a tap. No repeat BackBoard abort
has been observed during this swipe.


The subsequent capture validated all6,017,024 bytes and1,504,256 opaque pixels,
SHA-2568b7d115efc0efc2738edf784dd41c812fed6fd4284f9994699f428d61e38686f.
Visual inspection shows Touch Probe's white view, Taps:0, gray button at
physical x80..752/y904..1064, and a black home indicator. The lock-screen clock
and flashlight/camera overlays are gone. A diagnostic status banner remains
(rdar:45025538), and the button's title is still absent. This verifies a visible
app after the swipe, not a successful button press.

The four UIKit/gesture observers were removed after recording their final
15/15/1/0 hit counts. A separate input-only diagnostic converts the probe's
preallocated swipe samples to stationary normalized coordinates(0.5,0.55),
inside the visible button. It changes d1 at the full digitizer constructor for
both hand and finger events, checks every write, and stops at more than28
matches. It does not call the button callback or modify app counters/pixels.


## First verified app button interaction

The stationary-touch run completed exactly28 checked constructor writes,
14 dispatches returning1, and14 matching input-monitor callbacks including the
release at(0.5,0.55). The helper exited0 and reached its terminal marker.
TouchProbe180's log_event observer then recorded Taps:1. The constructor hook
was removed before taking a fresh capture.

The new capture independently decoded6,017,024 BGRA bytes with1,504,256 opaque
pixels, SHA-2564daf4bfdd9be1ade377592840c3325393d51ba8c7729c329c36195686b0d9353.
Visual inspection of touch-first-success-v77.png confirms Taps:1 on the real
UIKit view. No app counter, touch callback, hit-test result, or captured pixel
was substituted. This establishes one complete virtual-input -> UIKit button
callback -> newly rendered frame interaction in this diagnostic guest.

This is not yet a usable general emulator frontend. Native host pointer input,
continuous display presentation, restart automation, normal scene timing,
button-title rendering, and home/reopen remain unfinished. The stationary
input still uses a debugger coordinate conversion; a native tap mode should
replace it. The temporary attention, display bootstrap, identity, authentication,
and scene-watchdog workarounds remain part of this session. The earlier
BackBoard51 abort's cause remains unknown; no repeat occurred during the
successful swipe/tap sequence.


Cleanup: the temporary UIKit/gesture observers, stationary-touch constructor
hook, abort observer, and disabled extension stat hook were removed. The guest
was resumed. The established display/attention/bootstrap and finite scene
watchdog hooks remain active for continued work; this is not a clean-boot test.
