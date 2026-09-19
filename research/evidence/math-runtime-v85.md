# V85 MathTypesetting resource experiment

V83's live input agent exited cleanly, and QMP confirmed that guest paused.
It retains the verified browser-typed note, drawing, scrolling, and Home/reopen
demo. It has not been discarded.

V85 boots the staged V84-based image with all four matching MathTypesetting
resource files restored. It reached Bash after34.44host seconds. Kernel UUID
A5A62616-FA9C-337E-9C97-41BB2D427C63 and base0xfffffe002700c000 match V83.
The initial UserManager16 sample was still in dyld startup, with no cache mapping;
a subsequent sample of that same live PID resolved cache slide0x1cd0000 and
UserManager base0x100334000. All sampled threads were resumed.

The established bootstrap was rebased to those observed addresses. Kernel
inspection byte0xfffffe0017088db4 was restored and read back0 before UI services.
The scheduler-panic observer remains read-only and stopping. No arithmetic or
Calculator rendering assertion is bypassed. UI services are starting; the
resource fix and protocol4 wheel support still require runtime verification.

LaunchServices rebuilt successfully and reports Calculator installed. The
first-page Settings/Field Notes/Calculator IconState was uploaded, SHA256
verified, and installed before SpringBoard56 started. Guest sha256sum matches
all three restored MathTypesetting plists, and STIXTwoMath.otf is present.
Launchd resolved independently to0x104078000; its targeted ExtensionFoundation
stat ownership repair fired. The viewer is starting against protocol4.

Protocol4 announced and validated a real lock-screen frame after recovering a
corrupted initial UI transfer. An actual browser mouse drag from normalized
(0.5,0.98) to(0.5,0.2) sent14 pointer commands through the live frontend to unlock.

Validated frame7 shows the real Home screen with Settings, Field Notes, and
Calculator on page1. The browser mouse click at normalized(0.61655,0.11163)
now targets Calculator. The original saved-note guest remains paused.

The first Calculator short-tap command was interpreted as a long press and
opened its context menu; it did not spawn the app. This is a remaining input
timing rough edge despite preallocated tap events. The real menu now visibly
reads Remove App, confirming the SpringBoardHome resource localization fix.
Screenshot math-localized-menu-v85.png. The menu is being dismissed for retry.

After dismissing the long-press menu, the next actual Calculator icon click
reached its verified posix_spawn entry. Persona1003 was absent(errno3), allocated
through kpersona(type2/UID501,return0), then queried successfully(return0). The
experiment verified the saved SP and restored all borrowed register/scratch
state before disabling its two breakpoints. Calculator reached UIApplicationMain
with exact Apple binary UUID19A87853-DD17-33B3-99DB-5C8AB6F0EC48 at base0x104e80000.
A read-only, Calculator-UUID-guarded observer will record MathTypesetting data,
parsed configuration, and initialized environment values if those paths execute.

V85 arithmetic investigation: the actual browser clicked 2,+,3,=; subsequent
validated guest frames still have a blank expression/result area. AC changed
to C and operator/digit highlights appeared, proving input reaches app state,
but not proving a correct arithmetic result. Restoring MathTypesetting alone
has therefore not resolved the display problem.

Read-only UUID-scoped breakpoints observed CalculateUI.CalculateExpressionView
body evaluation after input, followed by its attributed-text formatting helper
(unslid0x20d22e860). The observed font size was71.4334 and target height97.
No observed hits yet at the separate ExpressionTypesetImage path or the three
MathTypesetting environment checkpoints. This narrows the next investigation
to the normal expression text/layout path; absence of those hits is not proof
that MathTypesetting is never used. No computation or rendering was replaced.

Further read-only captures prove real Calculator arithmetic internally: a
formatted result buffer decodes as U+200E followed by249, and its paired
expression decodes as U+200E246 U+200E+ U+200E3. These are the app's own
NSAttributedString buffers after browser input; the observer made no writes.
The fitted result measured126.264x92.15, and the expression91.774x38.95.
This is not completion: the visible display remains blank in that region.

CalculateUI's exact-build expression-view type metadata includes a _MaskEffect
with HStack, LinearGradient, and Rectangle. That makes masked compositing the
next diagnostic target. A read-only Calculator-scoped CALayer setMask: observer
is now installed at runtime0x18617d198; no masks have been changed.

The first mask observer encountered a nil mask and its diagnostic memory read
returned None, causing a Python callback exception and stopping the guest.
Register inspection confirmed setMask:nil (x2=0), not an app exception. The
observer now handles failed/nil reads, and the same guest was continued. The
first observed setter was clearing a mask on a toolbar-related layer; it is
not evidence of the expression mask's behavior.
