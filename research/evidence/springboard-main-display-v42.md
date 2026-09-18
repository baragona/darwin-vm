# SpringBoard reaches the main-display assertion after the passwd fix

After adding the firmware-matched public passwd file, the same managed job
passed the old initialization failure and exited with SIGTRAP rather than
SIGSEGV. A second managed launch using `launch-probe start com.apple.SpringBoard`
returned errno 0 and was caught by a hardware breakpoint at kernel
`0xfffffe002ab8e2a4`, conditioned on x0 == 6.

Observed exception type: 6 (breakpoint). Codes: `[1, 0x1bc9fa584]`.
LLDB unwound the exception frame to a user `brk #0` at `0x1bc9fa584`.
Shared-cache base `0x194c84000` was verified by its `dyld_v1  arm64e` header;
slide is `0x14c84000`. The crash message at user address `0x7912c6a911` was:

```text
Invalid condition not satisfying: mainDisplay
```

The symbolic stack is:

1. `-[FBSDisplayMonitor _initWithDisplays:mainDisplay:bookendObserver:transformer:].cold.5`
2. `-[FBSDisplayMonitor _initWithDisplays:mainDisplay:bookendObserver:transformer:]`
3. `-[FBSDisplayMonitor _initWithBookendObserver:transformer:]`
4. `+[FBDisplayManager sharedInstance]` initialization block
5. `_FBSystemShellInitialize`
6. `_SBSystemAppMain`

This identifies the missing main display as the current explicit assertion.
It is not inferred solely from absent hardware or failed service lookups.
The new stack is after the previous Chrono observer initialization in
SBSystemAppMain, supporting the resource fix rather than relying just on a
changed signal. RunningBoard and Chrono-service lookup warnings also remain,
but this trap's message names mainDisplay.

All hardware breakpoints were removed, LLDB detached, and the guest resumed.
No Developer Mode override was applied in v42. The current diagnostic launch
still uses the mobile user domain with Conclave removed; a faithful system
launch and graphical interaction remain unfinished.
