# Dictation notification experiment (V72, 24A437)

This is a negative diagnostic result, not a UI fix. The guest uses cache slide
`0x2e3c000`; SpringBoard is PID 127. The previous biometric service restoration
remains in place.

Repeated AppleKeyStore selector-17 requests were observed through
MKBGetDeviceLockState, UIKeyboardInputModeController.deviceStateIsLocked,
UIDictationController.dictationIsFunctional, and keyboard dock updates initiated
by dictation availability notifications. At UIKit return address `0x187d6e2e8`,
MKBGetDeviceLockState returned `0xfffffff3` (-13). The caller accepts only zero
or three as unlocked. No device-lock return value was changed.

## Two temporary, object-scoped experiments

1. At `-[AFDictationConnection _tellSpeechDelegateAvailabilityChanged]`, unslid
   `0x19909f2e4`, runtime `0x19bedb2e4`, return immediately before the prologue
   only when the untagged self pointer equals `0x7c7bc98700`. The breakpoint
   reported 1,144 hits before deletion. The completed capture remains uniformly
   gray, SHA-256 `18d3e4243754358002b629c52fda64305e8c1c9628146a1b66bd7f528c9cf0f8`.
   See `dictation-quiet-capture-v72.json` and its PNG/transcript.
2. Observe the queued callback block at unslid `0x1990987b0`, runtime
   `0x19bed47b0`. Block `0x7c8637f690` contained invoke address `0x19bed47b0`
   at offset 0x10 and the same tagged connection `0x0800007c7bc98700` at
   offset 0x20. Delete the first override, then temporarily skip this void
   callback only for blocks capturing that connection. Its breakpoint reported
   1,852 hits before deletion (including the initial observation).

The second capture attempt is invalid: the collector began while the debugger
was still paused, input was lost, Bash reported `64: command not found`, and the
collector timed out. It must not be used to infer display behavior. A UART
collector timeout does not stop the guest or cancel its pending commands.

Both notification overrides (breakpoints 17 and 18) were deleted. No code bytes,
preferences, or lock-state results were patched by these experiments. The kernel
inspection byte was left at its previously verified zero value.

## App-layout failure recurs

After removing the second override and continuing, the existing Objective-C
exception observer stopped at `0x18324c328`. The reason string read directly from
the exception was:

> The appLayouts array MUST contain the app layout we're transitioning to.

The first caller was `0x22794e984` (unslid `0x224b12984`), the same
SBMainSwitcherControllerCoordinator transition completion assertion seen in V71.
Next callers were `0x227db3e48` and `0x227f805d8`. The existing conditional
failed-membership observer at `0x22794e930` also reported one hit, but its operand
state was not saved, so the missing layout's identity remains unknown.

The guest was left paused at the exception for further inspection. The debugger
cannot reconstruct the caller's general-purpose registers in this stripped
remote session. Do not infer the missing layout from the exception alone.

The observed sequence does not prove that dictation caused the layout failure,
or that suppressing notifications improves responsiveness. The useful next
checkpoint is to capture the transition target and coordinator's actual layout
array before the assertion, and establish why their membership differs.
