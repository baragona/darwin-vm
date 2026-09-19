# V80 live display/input runtime

Booted `service-root-live-input-v80.dmg` with `live-input-v80.tc` after
pausing V79 through QMP (query-status confirmed paused). V80 uses the same
kernel, device tree, SPTM, TXM, and arguments as V79. It is the sole executing
Darwin VM; V79 is temporarily retained as a fallback.

The boot collector matched old Bash versions and missed the Bash 5.3 prompt.
The boot transcript independently proved shell readiness; only that collector
was terminated to free UART. The VM was not restarted.

UserManager PID16 thread sampling established shared-cache slide 0xbe74000
and candidate base 0x1007e0000. The persona callback verified Mach-O magic and
the seeded mobile UUID string. Launchd callback verified base 0x100e44000.
The kernel inspection byte was restored and read back as zero before UI startup.
The relocated startup hooks include the persistent 832x1808 display geometry
and synchronous hosted-layer setting used in V79.

The combined live agent is staged and its CDHash is in the trust cache.
Host protocol tests and strict cross-build/signature checks pass. Guest
rendering, input delivery, and browser interaction are not yet verified.

The persistent agent started successfully, registered its virtual touch service,
and emitted LIVE_INPUT_READY 1. The local viewer runs on 127.0.0.1:8989.
Early frames were correctly rejected because kernel/daemon logs interrupted
base64 lines. After the initial service storm, complete CRC-validated frames
arrived. The first inspected PNG was black; this does not prove a visible Home
screen. A Home event was submitted through the HTTP input endpoint; actual UI
delivery remains unverified.

SpringBoard repeatedly aborted during initialization. A breakpoint at abort
captured the actual message: Could not cast value of type _NSXPCDistantObject
to ExtensionFoundation._EXDiscoveryServiceProtocol. Logs independently showed
failed lookups for com.apple.extensionkitservice. The V79 extension directory
stat workaround disabled itself after one use; in this boot it had been used
before SpringBoard's connection. Re-enabling it persistently (still restricted
to that exact guest framework directory, uid99, directory mode0755, successful
stat) allowed extensionkitservice61 to start for SpringBoard60. SpringBoard60
then continued initialization. This is a guest diagnostic workaround, not an
upstream-ready ownership fix.

The next runtime checks are a visible Home frame, bounded input/frame latency,
short icon tap, edited note and drawing persistence, and Calculator. None is
claimed complete by the transport's acknowledgements.

Frame52 subsequently passed all integrity checks and visual inspection shows
the real iOS lock screen (clock, date, status bar, bottom Home indicator).
PNG SHA256 a685b05de55d3fa25b1b528609a60d4e97293ee82263f77c5534b934473f74be.
This verifies live-agent capture through the host HTTP endpoint. It does not
yet establish browser mouse delivery or acceptable interactive latency.

A native upward swipe submitted through the HTTP endpoint received three
successful guest acknowledgements (down, coalesced move, up). The resulting
frame must still be inspected; acceptance alone is not gesture delivery proof.
Host recompression of the verified lock framebuffer produced 57,905 bytes at
zlib level1, 31,074 at level6, and 28,681 at level9. This suggests testing
stronger compression in the guest to reduce UART time without changing pixels;
host timing does not predict emulated-guest compression cost.

The frame after the acknowledged swipe still showed the lock screen; unlocking
through the persistent endpoint is not established. A UUID-verified live-agent
compression experiment changed only its compress2 level argument to6. Four
measured calls took 24.16, 19.90, 10.83 and17.62 host seconds. Corresponding
compressed lengths were 28,957, 26,182, 27,180 and24,382 bytes. The experiment
and two timing observers were removed afterward, restoring the original level1
behavior for subsequent calls. Full-screen compression is a material part of
the latency, though this does not isolate rendering and serial output costs.

A protocol2 agent now builds with exact changed-rectangle updates. Unchanged
captures avoid compression; patches reference their exact previous frame, and
the host requests a full frame after corruption or missing history. Unit tests
verify exact pixels and reject patches against the wrong base. This is built
for V81 but has not been staged or tested in the guest yet.

A second swipe preserved all14 touch samples by placing ping barriers between
moves in the existing bridge queue. All26 commands were acknowledged. Frame65
then visibly showed the lock-screen sheet pulled upward, with the bottom controls
and Home indicator near the top of the screen. This proves touch movement reached
SpringBoard through the persistent endpoint; it is not yet proof of a completed
unlock. The first test had collapsed all intermediate motion into one sample.
The host now retains up to16 consecutive motion samples before coalescing excess
updates, preserving normal gesture paths while still bounding backlogs.
