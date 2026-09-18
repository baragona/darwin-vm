# Bypassing Setup for the UI prototype (V72)

The user prefers skipping Setup and reaching a usable shell/home screen. This
experiment follows that direction; completing the Setup application is no longer
the immediate target.

## Preference-only attempt: insufficient

The guest's com.apple.purplebuddy.plist contained only lastPrepareLaunchSentinel.
Static disassembly of BYSetupAssistantHasCompletedInitialRun (0x1cb136fe4) shows a
read of SetupFinishedAllSteps in com.apple.purplebuddy. The firmware also defines
SetupDone. Both booleans were set true while preserving the existing sentinel.

The original guest file was backed up to
/var/tmp/purplebuddy-before-skip-v72.plist. The replacement was live-uploaded and
hash-verified, installed at /var/mobile/Library/Preferences/com.apple.purplebuddy.plist,
owned by 501:501 with mode 600. Its SHA-256 at installation was
2be483323355759a80a296bd389197394bb856bfa891b08583c54c1d58aec0ec.
The budd and cfprefsd jobs were stopped before installation and restarted after.
These preference changes live on volatile guest /var, not the root disk image.

SpringBoard PID176 still had setup-required reason 1. At
-[SBSetupManager _isInSetupMode], runtime 0x2279a831c, self was
0x0d00007497027ae0 and its +0x38 field at 0x7497027b18 contained 1. The actual
method tests this field for nonzero. The display capture was opaque black.
The prior failed-membership and Objective-C exception observers fired again.
Therefore these flags alone do not bypass Setup in this guest configuration.

## Direct Setup requirement override

On the next SpringBoard launch, change the argument to
-[SBSetupManager _setSetupRequiredReason:] to zero before its prologue.
Unslid address: 0x224f112ac. V72 cache slide: 0x2e3c000.
Runtime address: 0x227d4d2ac. This setter stores x2 into self+0x38 and retains its
normal change-notification logic; it is not an override of the app-layout
assertion or an activation-service implementation.

LLDB commands for this specific boot (replace N with the assigned breakpoint):

```text
breakpoint set -H -a 0x227d4d2ac
breakpoint command add -o 'register write x2 0' N
breakpoint modify N -G true
```

The live breakpoint is 21. It reported two hits at the post-capture checkpoint.
The existing persona, credential, display/backlight, and per-launch extension
bundle bootstrap workarounds still apply. The extension stat UID was changed
only after verifying the ExtensionFoundation extensionkitservice.xpc path,
successful stat, mode0755, and original UID99; its observer was removed.

Fresh SpringBoard PID186 remained running through the completed capture. The
first read-only _isInSetupMode observer saw self0x0e00007993027b40 with reason0
at 0x7993027b78. This is evidence for the state bypass, not a usable home screen.
The captured display is still opaque black (SHA256
83385445671c64c9bb30b63aedc2afef4057fae423270db5f392e7be607205a8).
At that checkpoint the old exception and failed-membership observers each had
two total hits, all from earlier launches; neither had a new observed stop.

The direct override is temporary and remains active for further UI work. Remove
breakpoint21 and restart SpringBoard to remove it. Restore the preference backup
with budd/cfprefsd stopped to undo the preference experiment. No host security
settings were changed, and no firmware binaries are included in this evidence.

A second read-only setup-mode observation during the Home action, after both
setter hits, again read zero at 0x7993027b78. The UART collector timed out while
the debugger paused execution. After continuing, a passive collector observed
HID_PROBE_END and SpringBoard PID186 still running. Its transcript did not contain
an intact SERIAL_DONE marker, so shell completion is not independently verified
by that marker. No new Home-result screenshot was taken.
