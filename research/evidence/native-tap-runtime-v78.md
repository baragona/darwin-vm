# Native tap validation in V78

The guest image is an APFS clone of V77 with only /bin/hid-input-probe replaced.
The separately extended trust cache contains the new signed helper's CDHash.
The build report records both source and installed binary identities. No Apple
firmware binary is included in this repository. V77 is paused as a known-working
fallback during this test; the older four Darwin VMs were closed at the user's
request. The separate Windows VM was not changed.

The helper adds --virtual-tap X Y with finite normalized coordinates in[0,1].
It uses the existing virtual service, display association, preallocated14-frame
sequence, dispatch path, and explicit release. Coordinates are selected in C,
not rewritten by a debugger. Strict iOS compilation and signature verification
passed. A host-only dlopen stub exercised18 argument cases without opening any
host HID framework. In the guest, --virtual-tap nan 0.5 returned2 before loading
input frameworks, proving the installed helper recognizes the new command.

The initial QEMU invocation rejected -restore as a host option before starting
any VM. The corrected command passes -restore aks-endpoint=0 inside the kernel
boot-argument string. V78 then booted to Bash. Kernel load is0xfffffe002700c000;
the verified pointer at0xfffffe0027ea1600 identifies inspection byte
0xfffffe0017088db4. The byte was temporarily enabled for a UserManager thread
sample, then restored and read back0 before UI startup. The thread sample
completed with RESUME_RESULT=0 and revealed cache slide0x14c90000.

UserManager16's base0x10250c000 was derived from its sampled main frame, then
checked against Mach-O magic. The bounded object traversal found the existing
seeded mobile UUID NSString at0x73eec08820 and verified its36-byte UUID text
before borrowing it for the known persona lookup arguments. Launchd base
0x104358000 was similarly derived from the known xpc_bundle_get_path callsite
and checked for Mach-O magic. Its stat hook at0x104381d0c is restricted to the
ExtensionFoundation extensionkitservice bundle and verifies UID99/mode040755/
result1 before writing only the returned UID. After the first one-shot hit,
the hook was re-enabled for the subsequent startup clients; it does not alter
on-disk bundle ownership.

Services: BackBoard33, SpringBoard45, RunningBoard31, UserManager16, lsd25,
installd27, container manager23, cfprefsd36, MobileGestalt37, biometrickitd43.
The background budd job was registered; Setup's UI remains bypassed. App database
rebuild returned1, and an independent query reported Touch Probe installed at
/Applications/TouchProbe.app. A display query reported LCD ID1, main display
present, and mode832x1808. The relocated display, identity, authentication,
Setup, attention, and finite scene-watchdog workarounds were carried forward
from the V77 experiment; this is not a clean stock boot.

At Touch Probe's real posix_spawn entry, the kernel-persona query returned
ESRCH3 for ID1003. The existing controlled syscall experiment allocated the
managed type2 persona with UID501, and a separate query verified it. All saved
registers and1536 scratch bytes were restored and checked before resuming the
original spawn. The syscall and spawn observers were removed. TouchProbe51
then entered UIApplicationMain at Mach-O base0x104624000; the read-only app log
observer is at0x104628584.

An attempted open command overlapped an automatic debugger stop while its UART
text was being transmitted, leaving a shell fragment (00d6d96: command not
found). That transcript is not evidence of a completed second open. The earlier
open request had already progressed to the observed spawn; no app lifecycle
result is inferred from the fragment or its collector timeout.


## First capture and export timing

The existing QMP endpoint reported running before this test. Touch Probe's
read-only observer recorded SCENE_ACTIVE. A fresh capture completed with
DISPLAY_CAPTURE_RENDER_RESULT=1, 1,504,256 changed and opaque pixels, and
6,017,024 output bytes. The lossless compression helper subsequently hit its
15-second guest alarm; a standalone retry did too. Neither attempt exported
a viewable frame. A third export runs the same helper in a subshell with
SIGALRM ignored, limited to that subprocess; its observation deadline is
300 host seconds. This changes no framebuffer bytes or app state.

The relaxed export completed successfully: 27,110 compressed bytes, decoded
to all 6,017,024 BGRA bytes with 1,504,256 opaque pixels. SHA-256 is
03c3d6edcc225b7f15d1ff25f38565a140edc899a29b8e10a2b38e8253e0767f.
Visual inspection of native-tap-before-v78.png shows the lock screen still
covering the app. An unmodified --virtual-swipe command was issued next;
no tap or successful dismissal is inferred before its result is observed.

The first swipe registered its virtual display service and dispatched frames
0 through 4, but the helper's 45-second alarm terminated it with exit 142
before the release frame. This is an incomplete gesture, not a successful
unlock. The service lifetime ends with the helper. The retry uses a subshell
ignoring SIGALRM and a 300-second host observation deadline, keeping the same
input code and coordinates. This exposes a practical limitation of the
existing fixed alarms under this guest's current load.

The relaxed swipe completed all 14 dispatches with result 1, including
release, and 14 monitor callbacks; exit was 0. BackBoard33 subsequently
exited due to self-sent SIGABRT, confirmed in the full serial log, and launchd
started BackBoard60. The cause is unresolved; the successful HID dispatch
does not prove dismissal or app delivery.

The initial display-size hook was one-shot. Recovery installs breakpoint 22
at 0x1993f7978 with the same 416x496 condition and 832x1808 register values,
but leaves it available for future compositor restarts. The original virtual
display flag and name hooks remain active. A targeted stop/start of
com.apple.backboardd follows; no whole-VM restart was requested.

Recovery started BackBoard64. An early display-probe67 query itself aborted
in the QuartzCore query path; its transcript preserves the sampled stack.
It did not establish the recovered display mode. SpringBoard45 was then
intentionally stopped and replaced by SpringBoard69. Native tap delivery and
a post-recovery frame remain unverified; V77 is still the paused fallback.


## Recovery follow-up

A subsequent display query succeeded with LCD ID 1, mode 832x1808, and a
main display. The LS open helper returned false/exit 1, but the asynchronous
request subsequently spawned TouchProbe74 and its read-only observer reached
SCENE_ACTIVE. This again shows why helper results alone are insufficient to
infer the underlying app launch outcome.

A read-only abort observer is armed at 0x19cc088b0 (the prior boot's observed
abort entry relocated by the verified cache-slide difference). It records
registers and a bounded frame-pointer walk, then stops for inspection.
It does not bypass aborts.


## Reproduced compositor assertion

The recovered swipe delivered all 14 frames and release, with 14 monitor
callbacks. The abort observer then captured the graphics failure before
termination. The assertion string was:

    Assertion failed: (found != slist->end ()), function push_surface, file ogl-context.cpp, line 1309.

The frame walk reaches CA::OGL::Context::push_surface from
CA::OGL::AsynchronousNode::retain_surface, then repeated ImagingNode::render,
render_layers and LayerNode::apply frames. Return addresses were symbolized
at return-address minus four, avoiding mislabeling an assert call at the end
of push_surface as the following reset_statistics function.

The disassembly searches the context's surface list and asserts when the
surface cannot be found. We do not bypass this invariant. The next diagnostic
sets CALayerHost's setRendersAsynchronously: argument to false at its verified
entry (0x199205bb8), recording every requested value. Whether this avoids the
asynchronous renderer and fixes the failure must be tested. Continuing the
original abort allowed launchd to start BackBoard80; a SpringBoard restart
recreates hosted layers under the experiment.

The next SpringBoard instance (84) launched TouchProbe87, which reached
SCENE_ACTIVE. The synchronous-layer hook recorded two true-to-false writes
from caller 0x1996ea23c. The subsequent swipe completed with exit 0 and no
new abort recorded at that point. A native --virtual-tap 0.5 0.55 test follows.
This is evidence supporting further testing, not proof that the graphics
assertion is permanently fixed.

## Verified native button interaction

The native tap completed 14 stationary frames at (0.5,0.55), all dispatches
returned 1, and the monitor observed 14 callbacks including release. The
helper exited 0. TouchProbe87's read-only observer logged Taps: 1. No debugger
input-coordinate rewriting, app counter mutation, or callback substitution
was used. The subshell ignored the diagnostic helper's alarm for this test.

A new capture rendered all 1,504,256 pixels opaque. Its first serial export
was corrupted by interleaved logging and rejected by the strict decoder.
Re-exporting the saved compressed frame decoded all 6,017,024 bytes, SHA-256
4daf4bfdd9be1ade377592840c3325393d51ba8c7729c329c36195686b0d9353.
Visual inspection confirms Taps: 1 in the real app with the lock screen gone.
The hash matches the independently captured V77 result for the same UI state.
The diagnostic status banner and missing button title remain.

No additional abort was recorded during this swipe, tap, and capture after
the synchronous-hosted-layer hook was activated. This demonstrates one
successful sequence under that workaround, not long-term stability or
causal proof that it fixes every surface-list failure.
