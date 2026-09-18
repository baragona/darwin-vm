# Keyboard resources and the next startup dependency (24A437)

V71 boots the image prepared in `keyboard-image-v71.json`: the four restored
keyboard resource trees contain 1,464 verified files. The trust cache is unchanged.
The usual diagnostic UI bootstrap is still required; this is not a stock boot.
See the historical address snapshot `keyboard-startup-v71.lldb`.

SpringBoard PID 61 remained running through Home and a virtual swipe. The
objc_exception_throw observer had zero hits at the post-swipe checkpoint. V70's
nil return-key-label exception did not recur in this run. This supports restoring
the missing resources, but does not isolate which of the four trees was necessary.

The virtual touchscreen dispatched 14 frames successfully and received 14 monitor
callbacks. The post-swipe capture was a uniform dark gray 416x496 image with the
display on, not a usable interface. Its raw BGRA SHA-256 is
`18d3e4243754358002b629c52fda64305e8c1c9628146a1b66bd7f528c9cf0f8`.
The earlier capture was black. Neither image proves an application handled touch.

## Main-thread stall

Two thread samples found the same synchronous XPC wait on SpringBoard's main
thread. Symbolication is in `keyboard-dpi-stack-v71.json`:

1. `-[UIKeyboardDockView layoutSubviews]`
2. `+[UIKeyboardDockView _itemFramesForBoundingSize:]`
3. `-[UIScreen _nativePointsPerMillimeter]`
4. `-[UIScreen _ensureComputedMainScreenDPI]`
5. MobileGestalt / MobileGestaltExtensions
6. synchronous NSXPC reply wait

The kernel inspection byte was enabled only for sampling and restored to zero,
with a read-back check. Repeated key-store errors were also logged; the main-thread
samples identify a different wait and do not establish those errors as its cause.

`/usr/libexec/MobileGestaltHelper` exists in the guest (76,480 bytes), but its
launch plist is absent and `com.apple.mobilegestalt.xpc` lookup returns 1102.
The original firmware plist uses feature-flag dictionaries for the session and
user. `../mobilegestalt-probe.plist` selects the original mobile-user branch,
retains the Mach service, adaptive spawn type, and transactions, disables pressured
exit for observation, and adds diagnostic output paths.

The live guest's `/launchjobs` is read-only. Install temporary diagnostic plists
under `/var/tmp` for live submission; the first attempted system-volume write
failed and did not install a job. Submission from `/var/tmp/mobilegestalt.plist`
succeeded: MobileGestaltHelper PID 94 ran with LAST_EXIT=0, service lookup returned
RESULT=0 and PORT=7171, and its stderr was empty. See
`keyboard-gestalt-live-v71.txt`. A subsequent capture still had the identical gray
frame hash. Restarting SpringBoard with the service already present is the next
check; starting the helper alone did not visibly recover the existing session.

## Fresh startup with MobileGestalt available

Restarting SpringBoard also repeats its extensionkitservice bundle ownership
check. The one-time stat-result UID override is per launch request, not per boot.
Without repeating it, launchd rejected the original ExtensionFoundation XPC
bundle with error 147 and the restarted instances aborted. Do not interpret those
restarts as a clean test of MobileGestalt alone.

For PID 126, the helper check was observed again: the original extensionkitservice
path, helper result 1, directory mode 0755, UID/GID 99. Only that stat result's UID
was changed to 0 and the observer removed. MobileGestalt PID 94 remained running.
PID 126 survived the next capture with LAST_EXIT=0, but its screen was black.

It subsequently threw NSInternalInconsistencyException in
`-[SBMainSwitcherControllerCoordinator layoutStateTransitionCoordinator:transitionDidEndWithTransitionContext:]`:

> The appLayouts array MUST contain the app layout we're transitioning to.

See `keyboard-transition-exception-v71.json`. Continuing reached libc abort.
This is a new app-layout failure, not the old keyboard return-key-label exception.
The attempted follow-up thread sampling was interrupted by this exception and
does not prove that every MobileGestalt query is now healthy. Investigate the
target app's LaunchServices record and scene/transaction state next; do not merely
suppress the consistency check.

The inspection byte could not be restored while stopped in this user mapping.
A temporary breakpoint at the already observed kernel idle instruction allowed
the write; a read-back confirmed zero and the temporary breakpoint was deleted.

At the final checkpoint the SpringBoard job was explicitly removed successfully.
The guest remains running, MobileGestaltHelper PID 94 has LAST_EXIT=0, and its
service lookup still succeeds (PORT=7427). Its temporary plist is on `/var/tmp`;
the checked-in plist must be installed or submitted again on a fresh boot.
