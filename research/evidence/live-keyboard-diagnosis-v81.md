# V81 keyboard delay investigation

The completed thread-probe sample of Notebook PID62 reports task_for_pid success,
seven threads, and RESUME_RESULT=0. Main-thread candidate stack (runtime addresses,
shared-cache slide 0x268c000) includes:

| Runtime | Symbol |
| --- | --- |
| 0x1916fdaec | IOKit io_connect_method +520 |
| 0x2413094bc | AppleKeyStore __get_device_state +152 |
| 0x1b0d9a1e0 | MobileKeyBag __get_device_lock_state +236 |
| 0x1b0d9a0cc | MKBGetDeviceLockState +48 |
| 0x1875be2e8 | UIKeyboardInputModeController deviceStateIsLocked +20 |
| 0x187ea24f8 | UIDictationController dictationIsFunctional +48 |
| 0x187012944 | UISystemKeyboardDockController updateDockItemsVisibilityWithCustomDictationAction: +1608 |

Symbols resolved with ipsw dyld a2s against the matching local shared cache.
This supports investigating the keyboard's lock-state query, but one sample does
not establish the duration or sole cause of input delay. UART repeatedly reports
AppleSEPKeyStore errors for PID62 because the SEP is unavailable.

Independent display-capture-probe --wake reported display already on and captured
the original note plus caret, without the test x. Waking and allocating a fresh
capture surface did not expose the missing text. Later, the existing debugger
observer recorded both previously submitted Draw/Scroll button actions. Thus
those clicks eventually reached the application.

Kernel task-inspection byte was restored and read back as 0x00 before testing.
A reversible debugger breakpoint (24) at deviceStateIsLocked entry 0x1875be2d4
returns false, matching the intentionally unlocked guest. This affects that
UIKit method wherever invoked, not only Notebook. No Secure Enclave or remote
attestation is emulated by this experiment. Runtime effect remains to verify.

The live agent exited cleanly for the sample, and the host viewer and agent were
restarted with a fresh transcript /tmp/live-view-v81-uart-2.txt. READY 2 was seen.

The restarted viewer validated full frame1, showing the original note followed
by the browser's previously submitted `x`, and the status `Scroll mode`.
`live-typed-x-v81.png` preserves the actual guest pixels. This is the first
visible proof of browser keyboard delivery to the native text editor in V81.
There were no keyboard-lock callback hits at this point, so the bypass cannot
be credited with this result. Input was eventually delivered without that hook.
A second browser click at normalized (0.78,0.235) and KeyY sent D/U and HID usage28
press/release through the real frontend for another latency check.
