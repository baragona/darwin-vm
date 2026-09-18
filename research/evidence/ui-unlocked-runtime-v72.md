# UI-unlocked state requires an authenticated provider (V72)

The direct UI-lock experiment reached a new, specific consistency check:

> Attempt made to UI-unlock while not authenticated

This is not the earlier missing-Setup-layout assertion. It shows that changing
the UI-lock setter alone does not establish a valid unlocked session.

## Experiment

Restart SpringBoard with the existing Setup bypass. At
-[SBLockScreenManager _reallySetUILocked:], unslid0x22548d3ac,
runtime0x2282c93ac, a temporary auto-continue breakpoint set x2=0 before the
prologue. This preserves the setter's ordinary checks and notifications.
The usual ExtensionFoundation extension-bundle stat workaround was applied only
after checking the original path, helper result1, mode0755, and UID/GID99.

Fresh SpringBoard PID228 ran through the initial Home/capture command, with two
HID monitor callbacks. That initial capture was opaque black. At the first
checkpoint the new setter observer had zero hits, so that capture is not proof
of an unlocked UI.

During the subsequent wake/swipe, the setter had one hit. A read-only
isUILocked observer saw self0x0800007942ff4380 and read zero at its +0x190 byte,
0x7942ff4510. This verifies the UI flag changed, not authentication or successful
home-screen presentation. The setter then ran several more times before the
Objective-C exception observer stopped.

The exception object's reason string gives the assertion above. Its callers are
runtime0x2283a0178 (unslid0x225564178,
-[SBLockScreenManager _reallySetUILocked:].cold.1+60),0x2282c9538,
0x2282c9800,0x2282c5e30, and0x227e16bac.

## Authentication provider

The setter loads its provider from self+0xa8, sends isAuthenticated, and asserts
when asked to clear UI lock while that returns false. The observed provider at
0x7942ff4428 is0x0c00007943196e40. Its class is0x1eb683e48, which resolves in
SpringBoardFoundation to SBFUserAuthenticationController (unslid0x1e8847e48).

Relevant exact-build methods:

- isAuthenticated:0x1bb62bfd8 (tail-dispatches through an Objective-C stub)
- _isUserAuthenticated:0x1bb62df98
- _updateAuthenticationStateAndDateForLockState:0x1bb6546f0
- _handleSuccessfulAuthentication:responder:0x1bb655028

The internal getter uses cached/model state rather than just the UI-lock byte.
No authentication-provider value was changed in this experiment. The next step
is to inspect or model the no-SEP guest's authentication state, then exercise the
normal UI transition. Merely suppressing this assertion would not prove a
consistent unlocked session.

## Cleanup and current state

The temporary UI-lock override (breakpoint40) was deleted after inspecting the
exception. The one-shot lock getter observer has also expired. Setup bypass21
and the pre-existing bootstrap overrides remain. The guest is intentionally
paused at the exception for further inspection. The wake/swipe collector timed
out during the pause; this does not mean its guest command terminated. Resume
and inspect/cancel that command before sending new UART work.

Evidence: ui-unlocked-start-v72.txt, ui-unlocked-capture-v72.txt/.json/.png,
and ui-unlocked-swipe-v72.txt. No post-exception screenshot or usable home screen
is claimed. The iconservices lookup also failed during this launch; that alone
does not establish the cause of the empty content.
