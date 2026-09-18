# Biometric connection waits and live guest uploads (24A437)

V72's saved SpringBoard thread sample contains 61 workers in
`-[BiometricKitXPCClient initializeConnection] + 32`, waiting through
`objc_sync_enter`. The calling block is
`-[BiometricKitXPCClientConnection xpcConnection]`. This is a connection contention
observation, not proof that it alone explains the blank UI. The thread indices
and symbol addresses are in `biometric-baseline-v72.json`.

The guest lacks `/usr/libexec/biometrickitd`, and its `com.apple.pearld` service
lookup returns 1102. The original executable is 75,872 bytes and its CDHash
`f683f2e8bcf7a6a466380fb4d5779f2825285948` is already in the guest trust cache.

The diagnostic `../biometric-probe.plist` uses that original executable at a
temporary guest path and retains the original Mach service, transaction settings,
and interactive spawn type. It omits hardware-triggered LaunchEvents,
`_LimitLoadToDeviceTree`, and `_Conclave` for this explicit no-SEP experiment.
This does not provide Face ID hardware or make biometric authentication work.

## Verified live transfer

`../upload-serial.py` transfers files into an idle guest Bash UART without editing
the host-mounted root image. It requires exclusive UART ownership and the guest's
base64, sha256sum, stty, chmod, and mv utilities. It writes a unique staging path,
checks the guest's SHA-256 against the host source, then publishes the requested
destination. Existing destinations are refused. Mode defaults to 644; executables
require explicit `--mode 755`. This does not add trust for unsigned/new binaries.

The initial burst transfer under SpringBoard's heavy logging timed out. No staging
or destination file existed afterward. The pending shell input was cancelled,
terminal echo restored, and SpringBoard's retry loop stopped. A single-byte-paced
33-byte smoke upload at 200 bytes/s then passed exact hash verification.

On timeout, the guest remains live and may still be reading the here-document.
Inspect it before retrying. Terminal echo can remain off; cancel incomplete input
and restore `/bin/stty echo` before sending ordinary commands. Never treat an
unverified staging file as an executable or report it as a successful upload.

The full 75,872-byte original daemon transfer passed at 1,000 bytes/s with
single-byte pacing. Guest SHA-256 matched the original:
`91328f1a2e6ea8bb84f8e05502d9bb05dfbad88fe003d5f30ec8aa17467efac1`.
The 709-byte launch plist was also uploaded and verified. No trust-cache change or
reboot was needed. A negative test against the existing smoke-test destination
was refused before transfer, as intended. Evidence transcripts contain prompts,
hashes, and diagnostics; terminal echo was off for the firmware payload.

## Starting the service

Submission from `/var/tmp/biometric-probe-v72.plist` and explicit start succeeded.
The original daemon ran as PID 121, LAST_EXIT=0, its stderr was empty at the first
checkpoint, and `com.apple.pearld` lookup returned RESULT=0 / PORT=2819.
Service registration alone does not prove that every biometric request works.

SpringBoard was started fresh as PID 127 with the usual per-request extension
bundle stat override. Its first capture remained black. A subsequent thread
sample had 14 SpringBoard threads and zero frames at the previously observed
BiometricKit connection wait, compared with 61 waiting workers in the earlier
70-thread sample. The daemon had four threads with its main thread in the run
loop. SpringBoard's main thread was in QuartzCore's synchronous render request
during `UIApplication _firstCommitBlock`, rather than the earlier keyboard-image
lookup sample. This is a single post-restart observation, not proof of sustained
responsiveness or a usable display.

The kernel inspection byte was restored to zero with a read-back check, and the
temporary kernel breakpoint was removed. The repeat touch/display test completed:
all 14 virtual frames dispatched successfully, all 14 monitor callbacks arrived,
and the display reported on. The captured image remains uniformly dark gray,
SHA-256 `18d3e4243754358002b629c52fda64305e8c1c9628146a1b66bd7f528c9cf0f8`.
The input probe now completes, but this does not prove that an app handled the
gesture. A usable graphical interface remains unverified.

The uploaded daemon and plist live on the guest's volatile `/var/tmp`; the root
disk image was not changed by this experiment. On a fresh boot, repeat the upload
and launch steps or install these artifacts in the offline image. Start the
biometric service before SpringBoard and retain the other documented bootstrap
requirements.
