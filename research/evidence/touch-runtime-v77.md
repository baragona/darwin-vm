# First UIKit app scene in V77

**Follow-up:** a later recovery run verified a real button callback and a fresh
`Taps: 1` frame. See [the successful input experiment](touch-backboard-recovery-v77.md).
The findings below preserve the earlier runs and their limitations.

Touch Probe is registered and has produced a real 832x1808 display frame with
`Taps: 0` and its button background. This is progress beyond a home-screen icon;
input, the button title, lock-screen dismissal, and normal launch timing remain
unverified. No counter value or display pixels were substituted.

## Boot and registration

V77 cache slide is 0x2920000. UserManager38 has executable base0x10444c000,
verified by Mach-O magic. Traversing its manager global at base+0xe7398 yielded
mobile UUID NSString0x78d2c04460, whose text matched the seeded mobile UUID.
The existing borrowed-argument persona, virtual-display, credential, backlight,
Setup, authentication and UI-lock bootstrap was reapplied with fresh addresses.
The temporary guest inspection byte at0xfffffe0017088db4 was restored/read back0.
The virtual display constructor supplied832x1808; display-probe confirmed it.
A fresh LS query confirmed org.baragona.TouchProbe installed at its bundle URL.

LLDB crashed in its remote-register/unwind machinery during initial display
startup. QMP showed the guest stopped in debug state. Reconnecting resumed the
same guest; no VM restart was inferred from the serial timeout. The original
startup command later completed its marker in the authoritative serial logfile.

Launchd base0x104560000 was derived from LR0x10457a21c at xpc_bundle_get_path
and verified by Mach-O magic. The existing exact-path ExtensionFoundation stat
workaround changed only returned UID99 to0 at0x77b95f1970, after checking directory
mode0755 and helper result1. The observer was removed after success.

## Two distinct launch prerequisites

Restoring LifecyclePolicy moved launch past the earlier missing-domain-plist
error. A real launch job was then created, but posix_spawn returned ESRCH.
The original FBS/RBS error chain now ended in NSPOSIXErrorDomain3, not the
prior RBSAssertionErrorDomain2. An observer restricted to Touch Probe's path
read its spawn attributes: persona ID1003, UID/GID501, with explicit groups.
The manifest contains synthetic IDs but does not allocate kernel personas.

A controlled syscall experiment ran in the entitled guest launchd context at
that app's posix_spawn entry. The exact-build libc wrappers supplied the syscall
number and argument ABI. Apple’s public XNU persona header/syscall source supplied
the 348-byte v2 information layout; it was tested against this guest rather than
assumed sufficient from source alone:

- Query1003 returned ESRCH3 with carry set.
- Allocate1003 as a managed test persona (type2), UID501, name darwinvm-501-0,
  returned0 with carry clear.
- A separate query returned0 and the requested ID/type/UID/name.

The experiment preserved and restored1536 bytes below the original SP and all
saved general registers, SP, PC and CPSR, with readback checks. It used the existing
libc syscall instruction; no kernel code, app spawn result, or entitlement was
patched. The original spawn request resumed unchanged and launched TouchProbe75.
This provisions one experimental kernel identity, not the full stock persona model.

Sources: [persona ABI](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/persona.h),
[persona syscalls](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/sys_persona.c).
See touch-persona-syscall-v77.json and the historical V77-only helper script.
Do not reuse its addresses in another boot. LLDB's cached frame PC once disagreed
with its live PC register; raw remote p20/p1e/p1f packets confirmed the actual
spawn entry, LR and SP before the syscall experiment.

## Scene watchdog and visible frame

Processes75 and82 were terminated by the scene-create watchdog (10-second
allowance). Process82 reached UIApplicationMain. The app's absolute log-file
path did not produce a readable file, so a read-only observer on its log_event
function captured lifecycle messages instead.

The first attempted watchdog multiplier used a hexadecimal value in a floating
register API. LLDB interpreted it numerically, yielding4.6336410666108191e18,
not60. That run (PID85) is not evidence for a finite timeout. An initial attempt
to terminate it used absent /bin/kill; the subsequent Bash builtin kill succeeded.

The corrected callback passed decimal60.0 and read back60. Fresh process90,
base0x100688000, reached did-finish-launching, scene-connect, visible-requested
and scene-active. LaunchServices returned1. The callback scales scene watchdogs
during this diagnostic; it does not prove normal timing or responsiveness.

The first capture hit its30-second alarm after rendering and is not an exported
frame. A subsequent capture, with its diagnostic alarm extended to120 seconds,
completed6017024 bytes and passed exact zlib/frame validation. Frame SHA256:
8364541dad2351c601924c8f83e158342ddddf7802f7e3520e19873decae9c9c.
The PNG visibly contains Taps:0 and a gray button area. Its title is absent, and
status/lock-screen controls remain overlaid. Scene callbacks alone did not prove
this visual result; touch-scene-v77.png does. Input is the next check.


## Input experiment and current limits

A first swipe expired after its first down event. After removing the noisy
biometrickitd job in this disposable guest and extending the diagnostic helper
alarm to 180 seconds, a new swipe completed all 14 frames, including release,
with 14 matching input-monitor callbacks and exit status 0. This did not establish
that the lock-screen layer was dismissed.

A separate stationary touch targeted normalized (0.5, 0.55), inside the captured
button rectangle. A temporary observer at the guest digitizer-event constructor
changed only the probe's coordinates; all 28 parent/child constructions read back
y=0.55. The helper completed all 14 frames, release, and 14 matching callbacks.
The app's read-only log observer reported no Taps: 1 event. A fresh display capture
was byte-identical to the earlier frame (same SHA256 above): Taps: 0, no button
title, and lock-screen controls still overlaid. Thus input reached the input
monitor, but successful UIKit button delivery remains unproven. An overlay
intercepting input is a hypothesis, not an established cause.

See touch-button-input-run-v77.txt, touch-button-coordinates-v77.jsonl,
touch-button-capture-v77.txt, and touch-button-v77.png. The coordinate observer
was removed immediately after the test. Launch-error, app-event, scene-watchdog,
and helper-alarm diagnostic breakpoints were removed afterward; the guest was
resumed with its existing bootstrap hooks. Kernel persona 1003 remains allocated
until reboot, and biometrickitd remains stopped. This is a diagnostic session,
not a persistent or unattended emulator bootstrap.


## Narrowing the input delivery gap

Three additional complete probe runs finished with 14 dispatches, 14 matching
monitor callbacks, release, and exit 0, under the original 45-second alarm.
The first was a stationary touch made with the same coordinate-only observer;
the other two were unmodified swipes. No helper alarm extension was needed.

Read-only UIKit breakpoints (unslid plus V77 slide 0x2920000):

- UIApplication _enqueueHIDEvent: at 0x185adf6c8: 0 hits.
- UIApplication sendEvent: at 0x184e3f6b8: 0 hits.
- UIWindow sendEvent: at 0x184945140: 0 hits.

The two swipe runs additionally observed BackBoard geometry after lookup at
0x22ac97524: 56 hits total. The last run recorded all 28 geometry buffers,
each exactly [832,1808,1,0,0,0,1,1]. This verifies the larger display's geometry
lookup without changing geometry or input. The posting method previously
observed in V69, at 0x22ac9c650, had 0 hits in these two V77 runs.

The gap is therefore earlier than the observed UIKit delivery methods, with
BackBoard destination selection/suppression the next investigation target.
This does not prove all possible posting paths were covered or establish the
specific suppression cause. In particular, an overlay consuming UIKit events
has not been demonstrated. All five routing observers were removed and the
guest resumed. Counts and geometry are preserved in touch-route-counts-v77.json
and touch-route-geometry-values-v77.jsonl. The generic register logger's receiver
and event field labels are only meaningful at Objective-C entry points; at the
geometry interior breakpoint they are raw x0/x2, not object interpretations.


## Correction: alternate posting path reaches a live HID connection

Further read-only tracing overturns the inference that zero hits on the earlier
posting method imply no delivery attempt. V77 uses the poster block at unslid
0x22ac997c0. Hit-testing, destination filtering, and adding a destination all run;
the poster block processes two destinations per frame, 28 callbacks per swipe.
Captured target IDs are 0x78b1e241 and 0x33839f03, with client task-port name and
connection identifier 0x13e63. A Mach task-port name is not a process PID; its
owner has not yet been identified.

The delegate supports sendEvent:forTargetID:toClientConnectionIdentifier:.
BKHIDEventHitTestDispatcher reaches its valid-connection send branch at
0x22acd9e44, then BKSendHIDEventToClientWithTaskPort at 0x22abd4700. Its optional
hook pointer is NULL, so that hook is not suppressing these events. A non-NULL
client manager receives the sends. A later repeat observes:

- 0x22abd1154: all 28 client lookups have non-NULL x19.
- 0x22abd122c: all 28 retained HID connections have non-NULL x19.
- 0x22abd123c: all 28 connection-dispatch results are zero (success branch).

The dispatch stub resolves to IOKit's 0x18f07e75c. These results establish a
successful connection-dispatch return, not receipt by UIKit or successful input.
The next target is the receiving connection's owner and event handling.

An intermediate observation found zero generation fields 0xb001b/0xb001c in
the poster's pending-update path. That check occurs AFTER the delegate send;
it must not be treated as proof that ordinary delivery was suppressed.

All six probe commands completed their terminal marker, 14 frames including
release, 14 monitor callbacks, and exit zero with the original helper alarm.
No return values, destinations, event metadata, or geometry were overridden.
Temporary routing breakpoints were removed and the guest resumed. Raw records
and address hit counts are in touch-destination-registers-v77.jsonl,
touch-destination-summary-v77.json and touch-client-counts-v77.json. The observer
records raw registers; only its explicit destination decode uses the verified
BKTargetDestination field offsets. Earlier generic event/receiver labels are
not reliable object descriptions for optimized direct calls.


## SpringBoard receives UIKit events but its main thread is waiting

Read-only BKHIDClientConnection PID inspection (verified getter offset 0x20)
identifies PID 68, SpringBoard, for all 28 destination sends. This resolves the
previously unknown task-port owner; these events are not targeting TouchProbe90.
The shared IOKit queue callback runs for multiple clients, including one whose
registered callback is _UIEventFetcherReceiveHIDEventCallback. A direct observer
on that UIKit callback records 28 hits in a subsequent complete swipe.

Another complete swipe observes 28 hits each at the fetcher's ingestion block,
_UIHIDTransformer handleHIDEvent:, and UIEventFetcher addFilteredHIDEvent:.
UIEventFetcher drainEvents: has zero hits. Thus the earlier statement that UIKit
shows no receipt was too broad: its event fetcher receives the events, while
UIApplication/window delivery and queue draining remain unobserved.

Guest thread samples explain a likely reason for the undrained queue:
SpringBoard's main thread is in a synchronous NSXPC reply wait, reached from
SBBacklightIdleTimer _reconfigureAttentionClientAndReset: through
AWAttentionAwarenessClient setConfiguration:shouldReset:error:. Two later
samples retain the same stack. TouchProbe90's main thread instead shows its
run loop beneath UIApplicationMain.

BackBoard51 (the original launch plist hosts com.apple.AttentionAwareness) has
77 threads. Its thread72 waits inside AWRemoteClient setClientConfig:shouldReset:reply:;
thread75 is in another synchronous XPC reply wait, under BiometricKitXPCClient
connect, initializeConnection, detectPresenceWithOptions:async:withReply:,
BKFaceDetectOperation, AWPearlAttentionSampler, and AWScheduler armEvents.
See touch-attention-stacks-v77.json for available symbol mappings; unmapped
addresses are explicitly null rather than guessed.

Because biometrickitd had been removed earlier, the existing guest job was
resubmitted and started. Both launch replies returned errno0; the later job list
confirms biometrickitd.pearl PID120. SpringBoard still has the same main-thread
wait afterward, and the drain observer remains at zero. Restarting that daemon
alone therefore did not resolve the wait in the observed interval. It is not
established when the wait originally began or whether daemon removal caused it.
A targeted next experiment is disabling the guest's attention-sensing path and
restarting SpringBoard, preserving the input implementation.

All thread probes returned RESUME_RESULT=0 and commands reached their terminal
markers. The temporary guest task-inspection byte was restored and read back0;
all new event observers were removed and the guest resumed. Unlike the earlier
snapshot, biometrickitd is now running. No host security setting changed.


## Attention-sensing bypass experiments and restart prerequisites

The first targeted bypass returned immediately at entry to
SBBacklightIdleTimer _reconfigureAttentionClientAndReset: (unslid 0x224bd14dc).
Restarting SpringBoard68 also terminated TouchProbe90 and its process-scoped
extensionkitservice69. The fresh SpringBoard processes aborted in ExtensionFoundation
before reaching the attention hook. Serial logs again report the extension bundle
rejected with error147. The guest bundle directory cannot be chowned in place:
chown returned1 and Read-only file system.

Reapplying the existing exact-path launchd stat-UID workaround at base+0x29d0c
(base0x104560000), with mode0755, UID99 and helper result1 checked, restored
extensionkitservice139 for SpringBoard138. The four-byte UID write was at
0x77b95ec6b0; the callback disabled itself. The earlier broad abort breakpoint
was diagnostic only and removed after recording the stack.

SpringBoard138 reached the first bypass, but a subsequent main-thread sample
showed a different AttentionAwareness connection wait beneath
SBIdleTimerGlobalCoordinator _setIdleTimerWithDescriptor:forReason:.
A second void-method bypass at unslid0x224bcffe0 was installed before restarting
as SpringBoard145, with the extension ownership workaround ready. This second
bypass executed twice. However, the next main-thread sample found
CSUserPresenceMonitor _resumeAttentionAwarenessClient -> resumeWithError: ->
connect: -> synchronous XPC reply wait. The caller-by-caller approach therefore
does not cover CoverSheet's independent attention client.

A third diagnostic at AWAttentionAwarenessClient resumeWithError:
(unslid0x1b7d3abdc) returns false without entering the unavailable service. It
changes only x0 and the live PC at method entry; no input event or app callback
is substituted. SpringBoard152 was restarted with these diagnostics installed.
The first record in touch-attention-events-v77.jsonl predates adding PC/method
selection to the logger; it belongs to the original reconfigure hook. Reloads
reset the logger's count, so counts are not global totals.


With all three attention diagnostics installed, SpringBoard152 reached the
UIKit event-fetcher drain: 371 entries were recorded before removing that
observer to avoid slowing the guest. This proves execution reached the drain
method; it does not by itself prove that the app received any touch.
The fresh Touch Probe process (Mach-O base 0x100710000) subsequently logged
DID_FINISH_LAUNCHING, SCENE_CONNECT_BEGIN, SCENE_VISIBLE_REQUESTED, and
SCENE_ACTIVE. The LaunchServices helper nevertheless reported LS_OPEN_RESULT=0
and OPEN_EXIT=1; its result is not being treated as proof of app launch failure
or success. The app's own lifecycle observations are recorded separately.

These are temporary debugger experiments, not a persistent attention-service
fix. The two void-method bypasses also skip their other idle-timer work; the
shared resume hook returns false without populating an NSError output. No
claim about the sufficiency of the shared hook alone has been established.
The first display export immediately after restarting SpringBoard still
contained the capture buffer's 0xa5 prefill, so it is not a rendered frame.


The settled capture exported all 6,017,024 BGRA bytes with 1,504,256 opaque
pixels (SHA-256 8417cfe08610ee69b6e92e1ff54d9dec4bbf2edffd8d5fbd548c34c24d03fdfc).
Visual inspection shows the lock screen, including its clock and home indicator,
covering the app. This is not a fresh visible Touch Probe scene and cannot be
used as evidence that the app button is reachable. The next input diagnostic
therefore uses the original upward swipe rather than the stationary button tap.


The post-bypass upward swipe completed all 14 virtual dispatch calls with
RESULT=1, observed 14 monitor callbacks including the release, and exited0.
However, immediately afterward launchd recorded BackBoard51 self-terminating
with SIGABRT and respawned it as BackBoard163. The transcript does not establish
the abort's cause or whether it was caused by the swipe. No app tap callback
was observed. The last validated image remains the pre-swipe lock screen;
lock-screen dismissal and app interaction are still unverified. A subsequent
SCENE_ACTIVE app message was recorded during recovery, without a fresh capture.

The guest is running with the attention and finite scene-watchdog diagnostic
hooks retained. The high-volume drain observer was removed. No host security
setting was changed. Raw serial transcripts retain their original CR bytes;
touch-attention-open-result-v77.txt is a CR-normalized excerpt of the serial
log covering the pending open command through its completion marker.
