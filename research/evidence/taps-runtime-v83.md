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

The fixed bridge accepted all14 browser unlock commands; the guest acknowledged
them and subsequently rendered Home (taps-home-v83.png). Initial Home still
shows Settings only, so pre-registering apps did not put Notebook on page1.
The prior V79 IconState evidence places Notebook on page2, with Calculator in a
Utilities folder. A six-move horizontal browser swipe again entered edit mode,
so streamed gesture timing remains a separate problem from the dead writer.

An exact-agent-UUID-guarded debugger experiment changes only compress2's level
argument from1 to6. The agent UUID is D68A368D-F424-336F-8858-80A3CE81EE93 and
its current base is0x1048c0000. No pixels or frame validation are changed. Six
frames decoded with zero rejections at the initial observation; later console
interleaving still caused two rejected transfers. Host recompression of identical
validated canvases shows roughly2–3x fewer bytes at level6; this is not a
controlled guest CPU-time benchmark. See taps-compression-comparison-v83.json.
The running guest uses the guarded hook; its on-disk agent still requests level1.

An actual browser click on Home's edit-mode checkmark generated the single
command T0.82300,0.03200. The guest accepted it and the next validated frame
removed the edit controls (taps-checkmark-result-v83.png). This verifies a real
UIKit/SpringBoard response to the short-tap path, beyond protocol acceptance.
Streamed swipes remain unreliable and are not claimed fixed.

The source now defaults to compression level6. A new strict iOS cross-build and
signature verification pass (taps-level6-build-v83.json); that new binary is not
installed in the running image. V83 continues with its original signed binary
and the UUID-guarded level6 hook. The source change needs staging in the next
image to remove that experimental debugger override.

On resuming work, QMP confirmed V83 running and the viewer reported fresh
unchanged frames. A native swipe helper completed14 accepted events, but a new
agent captured black. The authoritative launchd log shows SpringBoard49 had
already exited with SIGTRAP before the native swipe began. Consequently that
swipe cannot establish navigation behavior. A fresh frame timestamp alone does
not prove the Home process is alive: the prior agent retained its last surface
pixels. The guest itself did not panic. A bounded600-second CPU trap trace was
enabled before restarting only the confirmed-exited SpringBoard service.

SpringBoard restarted as PID63, rendered the lock screen, and an actual browser
unlock returned to Home. Before a native page swipe, launch-probe independently
reported SpringBoard63 live. The helper then delivered14 acknowledged HID frames
and completed. The bounded600-second trace finished with no breakpoint or
undefined-instruction exception; it disabled QEMU logging on exit. The earlier
SpringBoard49 crash has not been reproduced within this trace window. The native
swipe's visual result is now being inspected.

Current V83 IconState.plist was exported and its SHA256 independently matched
the guest: f08963aa99580f18bdac3ed78a109b9c9aefb2d0f393c08545e58d5d6bf32d5a.
It confirms Notebook on page2 and Calculator in Utilities; the native swipe's
fresh capture still showed Settings on page1. To expose direct app-icon targets,
SpringBoard63 was terminated with SIGTERM and its original layout retained as
IconState.before-demo-v83.plist. No DesiredIconState.plist existed in this boot.
A new423-byte binary plist moves Settings, Notebook, and Calculator onto page1,
leaving TouchProbe on page2. Upload and installed SHA256 both verify as
212e11e2c2e41cd88ea6e3c1ed29395ed9f052e847947a0a4f052960c47457fd. An initial
serial command was cancelled after an unintended debugger pause; independent
hashes confirmed the original was untouched before the successful retry.
SpringBoard restarted as PID91. The VM itself was not restarted.

Notebook's new read-only UIApplicationMain observer validates exact binary UUID
A52507A3-968B-3B3D-9C11-0AAEFC5D2B07 before installing its app-log observer.
The first-page layout and Home-icon launch now need visual verification.

A validated live frame now visibly shows Settings, Field Notes, and Calculator
on page1 (taps-demo-home-v83.png). An actual browser mouse click at normalized
(0.385,0.11) sends T to the Field Notes icon. App startup is being observed.

The actual browser tap on the first-page Field Notes icon reached Notebook
spawn. Persona1003 queried absent (errno3), allocated through kpersona as
type2/UID501 (return0), and queried successfully (return0). The controlled
syscall preserved the original launchd stack pointer and restored every saved
register and scratch byte before disabling both experiment breakpoints.
Launchd then recorded Notebook PID96, and the UUID-verified observer recorded
UIApplicationMain at base0x102bcc000, LAUNCHED, and SCENE_VISIBLE_WITH_IMAGE.
The last browser screenshot at this checkpoint still showed the black launch
surface; app rendering and input remain to be visually verified.

Validated full frame7 reached the actual browser and visibly shows Field Notes,
its default text, Save and Draw/Scroll buttons, bundled UIImage, and canvas.
This completes the first verified Home-icon mouse tap to rendered real UIKit app
in V83. Screenshot: taps-notebook-visible-v83.png. Keyboard editing follows.

Browser keyboard events sent Command-A and `hello from ios 27.` after clicking
the editor. The app later recorded18 Unsaved changes callbacks and both submitted
Draw/Scroll actions. There was substantial delay; neither acknowledgements nor
these callbacks independently prove the exact final note text. The first raw Q
did not close the agent; an attempted thread-sample command was rejected by the
still-running agent, so no thread sample was obtained. A clean newline followed
by Q produced LIVE_INPUT_CLOSED. The temporary kernel inspection byte was
restored and read back0 at a kernel stop before restarting the viewer.

The restarted viewer validated a fresh frame showing the exact browser-typed
text `hello from ios 27.` and Scroll mode. Screenshot taps-notebook-typed-v83.png
proves native UITextView editing beyond app callbacks. Drawing/save/reopen next.

A browser mouse stroke reached the app and its first segment rendered. Save and
the remaining stroke were delayed. A successful PID96 thread sample resumed all
five threads (RESUME_RESULT=0); the sampled main thread was formatting an NSError
for ManagedConfiguration MCDataFromFile, not shown waiting inside AudioSession.
Symbols are recorded separately; this one sample does not establish the root
cause. No audio-service change was made. Inspection byte restored/read0 again.
The app subsequently logged additional drawing callbacks and two successful
Saved on this iPhone events. The viewer has restarted to verify the final pixels.

Fresh validated frame1 now visibly shows `hello from ios 27.`, the complete
mouse-drawn check mark, and Saved on this iPhone. The note, bundled image,
buttons, and actual guest-rendered drawing are preserved in
taps-notebook-saved-drawing-v83.png. Home/reopen verification follows.

The actual viewer Home button returned to SpringBoard. A corrupted transition
frame was rejected, then full frame4 validated with Settings, Field Notes, and
Calculator icons. Screenshot taps-notebook-return-home-v83.png. The real
Field Notes icon was clicked again to verify reopening.

After clicking Field Notes again, validated frame6 shows a black app surface.
No Notebook exit is recorded. A read-only UUID-guarded MCDataFromFile entry
observer captured five main-thread stacks: AFDictationConnection availability
notification -> UIKit dictation availability -> AFDictationRestricted ->
MCProfileConnection isDictationAllowed -> migration-state file read. This
identifies repeated dictation checks, but does not prove their sole causality.
The read-only observer was disabled. A reversible, current-Notebook-UUID-scoped
breakpoint at AFDictationRestricted entry0x1adfba69c returns true to report
dictation restricted in this guest. It has fired successfully; effect on app
reopening remains unverified. It does not enable audio or emulate attestation.

The same live browser now shows Field Notes reopened from its Home icon with
the exact `hello from ios 27.` note, completed check-mark drawing, and Saved on
this iPhone status. Screenshot taps-notebook-reopened-v83.png verifies the full
Home-return/reopen round trip. The app was not killed, so this is foreground
reopening rather than a new process loading the file. The dictation-restricted
hook was active; a subsequent proposed availability-notification hook was never
installed, because the existing experiment was followed by lifecycle progress
and the recovered scene. This temporal sequence is not a controlled latency
benchmark or proof that dictation checks were the only delay.

After reopening, an actual browser click selected Scroll mode. A12-step mouse
drag upward moved the UIImage and saved drawing upward, scrolled the note out
of view, and exposed the vertical scroll indicator. Screenshot
taps-notebook-scrolled-v83.png verifies real UIScrollView input and rendering.
The Field Notes interaction sequence is now visually verified; Apple
Calculator arithmetic remains incomplete.
