# V82 CoreGlyphs resource test

V81's last browser Home swipe entered edit mode instead of changing pages;
Home-icon reopening is still unverified. The saved note and drawing remain in
that guest. Its live agent exited cleanly and QMP independently reports paused,
running=false. It is retained as a state-preserving fallback, not executing.

V82 boots service-root-glyphs-v82.dmg with glyphs-v82.tc and the same established
kernel/device-tree/SPTM/TXM inputs. The boot collector reached Bash after50.70
host seconds. LLDB identified kernel UUID A5A62616-FA9C-337E-9C97-41BB2D427C63 at
0xfffffe002700c000. The inspection pointer at0xfffffe0027ea1600 was read as
0xfffffe0017088db4, initially0; temporarily set and read back1 for bootstrap
sampling. UserManager started as PID16. The byte must be restored before normal
UI testing. Calculator is not yet verified in this boot.

UserManager sampling returned cache slide0x8978000 and a base+0x9980 frame at
0x10052d980, yielding executable base0x100524000. Bootstrap hooks were rebased
from the verified V81 set; UserManager hooks use its independent executable base.
The inspection byte was restored and read back0 before starting UI services.
Launchd discovery independently found base0x1042bc000 and installed its targeted
ExtensionFoundation stat hook as breakpoint22. Container/LaunchServices/installd,
RunningBoard, biometric, keyboard, and backboard services were submitted using
the existing guest jobs. SpringBoard startup is now being observed.

The first app rebuild timed out via the helper's alarm while SpringBoard45 was
running; the following query was cancelled with UART SIGINT and a Bash prompt
was observed. Pausing only SpringBoard45 allowed the retry to complete:
LS_REBUILD_RESULT=1, Calculator LS_IS_INSTALLED=1 at /Applications/Calculator.app.
The next command resumes SpringBoard before requesting Calculator launch.
This independently reproduces the service-contention observation; the narrow
telephony/Sleep hooks do not fully eliminate it.

The first open helper reached its30-second alarm. The retry ignores that
helper-specific alarm and reached Calculator's verified posix_spawn entry.
At launchd base0x1042bc000 with callerLR0x1042e21f4, the controlled persona query
returned errno3 for1003. Allocation(type2,UID501) returned0; an independent query
then returned0. Every borrowed register and scratch byte was restored and checked;
spawn/syscall-return breakpoints23/24 were deleted before resuming the app.
The old CoreGlyphs trap breakpoint25 remains armed at runtime0x190fcf2f0. A bounded
QEMU trap trace is active for the resource-restored launch.

Calculator54 reached UIApplicationMain. In-memory UUID matches the unchanged
Apple app at base0x102da0000. Read-only observers, guarded by this exact Mach-O
UUID, now watch the nonnil public-bundle branch and completed asset-manager
initialization. They disable themselves after logging rather than skipping any
Calculator code. The live browser bridge is being started against V82.

The first validated832x1808 frame shows the lock screen with flashlight/camera
glyphs, not Calculator's scene. The browser driver reconnected to its existing
Chrome process after the prior automation connection detached; no browser or VM
was restarted solely due to that connection error. A fresh page loaded the live
viewer, and an actual mouse upward drag was sent to unlock the guest.

The browser unlock reached the Calculator scene. Both UUID-guarded observers
fired in Calculator54: public bundle0x7db8df28f0, initialized asset manager
0x7db90c1800. Neither callback changes registers or app instructions. Thus the
resource restoration fixes the specifically observed nil-bundle startup trap.
The rendered frame after unlocking is still black (calculator-after-unlock-v82.png);
there is no verified Calculator UI or arithmetic yet. Further startup/rendering
investigation remains necessary; surviving the old trap is not completion.

Validated frame7 subsequently shows the real Calculator interface, including its
digit/operator grid and orange history/mode glyphs. The prior black frame was
not the final rendered state. Button fills and some bottom-area pixels remain
rough. Screenshot calculator-interface-v82.png records this visible UI milestone.
Actual browser mouse clicks now request2,+,3,= at the displayed control positions;
arithmetic remains unverified until the resulting display is inspected.
The600-second CPU trace ended cleanly without a breakpoint/undefined exception.

The initial2,+,3,= browser input acknowledgements did not immediately produce a
visible result. The live agent exited cleanly, and a new foreground activation
request made while unlocked returned LS_OPEN_RESULT=1. The bridge was restarted
with /tmp/live-view-v82-uart-2.txt to inspect the resulting screen. This separates
initial process startup from a confirmed accepted foreground request; it does
not yet establish arithmetic success.

The replacement bridge produced no UART output. Inspection of the authoritative
serial log found a terminal kernel panic, not an observation timeout:
`Non-monotonic time: last_dispatch at 0x6683457d, ctime 0x668341ce`
at sched_prim.c:815, followed by nested panics. Both guests were independently
confirmed paused by QMP. V81 remains preserved. Calculator arithmetic is still
unverified. The completed CPU exception trace predates this panic.

Read-only disassembly in the stopped V82 guest shows the failing comparison at
0xfffffe002abf6864: processor last_dispatch (+0x70) versus the timestamp returned
through the stack by 0xfffffe002abe5998. That routine samples AGTCNTVCT_EL0
(S3_4_C15_C11_7) and adds a per-CPU offset at +0x58, retrying if the offset changes.
QEMU maps the Apple virtual-counter aliases and CNTVCT_EL0 to the same
gt_virt_cnt_read function. The 943-tick discrepancy does not yet establish whether
the emulated counter, guest offset, or scheduler state caused the inconsistency.
No timer clamping or scheduler assertion bypass has been applied.
