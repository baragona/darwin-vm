# Synthetic frontend authentication advances visible UI (V72)

This experiment combines the existing Setup bypass with two temporary debugger
overrides in the guest: SBFUserAuthenticationController.isAuthenticated returns
true, and SBLockScreenManager._reallySetUILocked: receives false. This models
frontend state only. It does not implement SEP, decrypt a keybag, establish a
real authentication session, or prove a usable home screen.

## Overrides and fresh launch

After recording the previous authentication assertion, resume that failed
SpringBoard launch, cancel its pending UART diagnostic, and start fresh. The
new process is PID238. Existing persona/credential/display bootstrap workarounds
remain, including the verified per-launch ExtensionFoundation stat UID fix.

The authentication observer is at unslid0x1bb62bfd8, runtime0x1be467fd8. This
entry tail-dispatches before any PAC/stack prologue. Its Python callback sets
x0 to1, sets PC to the current LR, and returns False to continue. Live breakpoint43:

```text
breakpoint command add -s python -o "frame.FindRegister('x0').SetValueFromCString('1'); frame.SetPC(frame.FindRegister('lr').GetValueAsUnsigned()); return False" 43
```

The UI-lock observer is at unslid0x22548d3ac, runtime0x2282c93ac. Live breakpoint44
sets x2 to0 and auto-continues. Neither override is object-scoped; their scope is
the corresponding guest framework method. They are temporary research aids,
not production emulator implementations.

At the first post-capture checkpoint, authentication breakpoint43 had3 hits and
UI-lock breakpoint44 had1. The exception observer remained at3 total hits from
prior launches; the failed-layout observer remained at2. A subsequent read-only
isUILocked observer saw self0x040000746e404380 and its +0x190 byte at
0x746e404510 contained0. This verifies the UI flag independently of the setter
argument, but does not validate the authentication controller's whole model.

## Capture discipline

The first completed command included Home (2 callbacks) and swipe (14 callbacks).
Its display remained off and capture reported CHANGED_PIXELS=0, despite the
render API returning success. The exported buffer's zero alpha and placeholder
contents are not rendered UI. Its decoded hash419365b7d49cabe70d03dc068b96e2fc588b3438456ad351ead87fbceae73c7c
must not be presented as a graphical success.

A second command overlapped the inspection pause and produced no expected
capture output. Its collector timed out. QMP independently reported running.
After the collector ended, cancel incomplete shell input, restore stty echo,
and verify GUEST_READY plus the unique completion marker before new commands.

The clean capture subsequently reported display on and25,204 colored pixels.
Its first export timed out mid-payload. Re-export the unchanged packed file,
without recapture, using authenticated-ui-export-v72.txt. That export completed
with its marker and passed exact zlib/frame-length validation.

The inspected authenticated-ui-visible-v72.png shows SEARCHING and battery at
the top, two dark circular lock-screen controls near the bottom, and a white
home indicator. The remainder is black. It is still cover-sheet UI, not an app
grid. Raw BGRA hash:
c624a795acc2a8680f0f1a40942cecf1697c6f6ee532b5234f34b67d186e0cc9.

The visible changes follow a fresh launch and combined overrides; this does not
isolate the contribution of each override from elapsed startup time.

## Fresh swipe after controls became visible

The completed authenticated-ui-dismiss-v72.txt sequence woke the display,
delivered14 matching swipe callbacks, and captured another opaque frame with
77,334 colored pixels. Its SHA256 is
935d6d538d66cf67d2233bc5b64d822b46bcc76a3d2b0ab084f1a571bad95656.
The PNG shows two large white rounded shapes on black, rather than the previous
status bar, circular controls, and home indicator. This is a visible state
change after the gesture, but incomplete rendering and elapsed startup time
prevent identifying it as a working home screen from pixels alone.

A later Home action completed. A read-only observer on
SBCoverSheetPresentationManager.isPresented (runtime0x227a13474) had zero hits
and was removed; it did not establish cover-sheet dismissal.

Guest /Applications lists only Setup.app. There are no ordinary apps there to
populate or test the launcher. The matching original Preferences.app has16 files
and4,415,215 logical bytes. Restoring a real app is a concrete next integration
step; this finding does not by itself explain all missing graphical content.

The guest is left running as SpringBoard PID238. Frontend-authentication
breakpoint43, UI-lock breakpoint44, Setup breakpoint21, and earlier bootstrap
workarounds remain active. Remove43/44 and restart SpringBoard to undo the two
new prototype overrides. No keybag result, encryption state, SEP implementation,
or host security setting was changed. A usable graphical emulator remains
unverified.

At the final checkpoint, authentication breakpoint43 had375 hits and UI-lock
breakpoint44 had5. Exception and failed-layout observers remained at3 and2
respectively, with no new observed assertion stops during PID238's run.
