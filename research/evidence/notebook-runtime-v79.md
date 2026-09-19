# V79 notebook and Calculator runtime work

V79 booted service-root-notebook-v79.dmg with notebook-v79.tc, using the same
kernel, device tree, SPTM, TXM and boot arguments as V78. V78 is paused as a
fallback. No other project VM was changed.

The initial UART collector missed a split Bash prompt and was stopped after
the boot log established shell readiness. The first UserManager submission
was truncated by a debugger interruption while typing and failed with a missing
plist; the clean retry succeeded. This is not evidence of an image defect.

UserManager18 sampling established cache slide 0x1061c000 and candidate base
0x102440000. Its persona callback later verified Mach-O magic and the seeded
mobile UUID NSString before borrowing that argument. Launchd base 0x10242c000
was independently verified by its callback. Kernel inspection was restored to
zero before UI startup. The startup file relocates the prior UI workarounds;
its display-size hook is persistent and synchronous hosted layers are enabled
before app launch. The app-main observer only logs registers and does not
assume the earlier TouchProbe binary layout.

LaunchServices rebuild returned 1 and independent queries reported both
org.baragona.Notebook and com.apple.calculator installed in /Applications.
A display query confirmed LCD ID1, 832x1808, and main-display presence.
SpringBoard47 aborted during initial startup; SpringBoard49 replaced it.
The cause of that initial abort has not been established.

The first notebook-open returned false while SpringBoard initialized. A later
open helper hit its diagnostic alarm, but the actual asynchronous request
reached launchd posix_spawn. At that point the kernel persona1003 query returned
ESRCH. Controlled allocation of type2/UID501 succeeded, and an independent
query verified the result. The experiment restored all saved registers and
scratch bytes before continuing the original spawn, then removed its two
breakpoints. Notebook56 subsequently entered UIApplicationMain, LR0x1049b8828;
the locally built binary has the matching callsite return offset0x4828.

Registration and UIApplicationMain are not proof of a rendered usable app.
Rendering, text input, drawing, saving/reopening and Calculator arithmetic are
still pending runtime verification. Current diagnostics are not a live frontend.

The read-only app log observer at base+0x5e00 subsequently recorded LAUNCHED
and SCENE_VISIBLE_WITH_IMAGE. The unmodified virtual swipe dispatched all14
frames, including release, with14 monitor callbacks and exit0. A fresh capture
returned render1 with all1,504,256 pixels opaque. Export and visual inspection
are still required to establish the visible screen.

The first export strictly decoded all6,017,024 BGRA bytes; SHA256
955242d334aa66ae1e9af593b84e874b62147ea742c41dd46c9f650bbc88fef9.
Visual inspection confirms the Field Notes heading, Save and Draw/Scroll labels,
initial note, embedded prior Home capture, and gray drawing canvas. The
diagnostic rdar banner remains. This proves the app's initial visible scene,
not editing, scrolling, saving or restored content.

Native tap(0.6,0.13) switched the app into drawing mode, recorded by the
read-only log observer. Native drag(0.2,0.78)->(0.8,0.9) delivered the app's
touch handlers, which logged drawing changes and then Saved on this iPhone.
Both gestures dispatched14 frames including release with14 monitor callbacks;
the command exited0. The save message is emitted only when atomic plist
writeToFile returns true. This does not yet prove reload/persistence.

The post-drag framebuffer strictly decoded6,017,024 bytes with all pixels
opaque, SHA2568347089c575d8d4e8dea5d8c9e0f47f36d29fc9fc12af6fc261cf6a8ca7eba11.
Visual inspection confirms a black diagonal stroke at the requested canvas
coordinates and the Saved on this iPhone status. This verifies native button
and drawing interaction in Field Notes and a successful app-reported write.
A process-relaunch test remains necessary to prove restored saved content.
