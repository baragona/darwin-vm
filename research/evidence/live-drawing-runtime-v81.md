# V81 browser drawing and responsiveness comparison

The pending Draw button eventually executed while the input agent remained
alive. Thus service closure is not required for delivery; the earlier temporal
association is insufficient to explain the delay.

Using the actual viewer page in Chrome, a mouse drag sent D at (0.18,0.76),
12 intermediate M positions, and U at (0.78,0.91). Normalized coordinates map
to the visible drawing canvas. The scene initially remained unchanged.

The host viewer and guest agent were stopped cleanly. Bash paused only
SpringBoard44 using SIGSTOP. The app observer then recorded three Unsaved drawing
messages followed by Saved on this iPhone. This experiment also changed input
service lifetime, so it is not an isolated proof of SpringBoard causality.
The restarted viewer validated frame2: the saved `A note from my virtual
iPhone.xy` and a black diagonal stroke are visible in live-browser-drawn-v81.png.
The pixels are from the guest renderer; the host did not draw the stroke.

Installed reversible debugger breakpoint26 at0x2273865e0. Matching shared-cache
symbol lookup identifies unslid0x224cfa5e0 as SBTelephonyManager
queue_setFastDormancySuspended:withConnection:. The callback skips this void
method because the guest has no CommCenter backend. It does not emulate cellular
service. Its impact must be verified after SpringBoard resumes. Existing hooks24
(keyboard unlocked state) and25 (Sleep retry) remain installed.

Prepared qemu-trap-trace.py for Calculator. A three-second live smoke test
collected10565 SVC,1999 Data Abort,6994 genter,404 FIQ exceptions without parser
errors. It disabled logging afterward. No breakpoint/undefined exception occurred
in this smoke window; Calculator was not launched. The FIFO avoids retaining all
ordinary syscall logs on disk; only trap records (capped1000) and counts persist.

SpringBoard was explicitly resumed with SIGCONT, confirmed by the shell marker
in live-sb-resume-v81.txt. The telephony hook then fired repeatedly at the two
callers seen in the original stack; this does not yet prove a latency fix.

Calculator was launched via /bin/ls-open-probe --open com.apple.calculator.
Launchd spawned PID81, although the helper's LS_OPEN_RESULT was0. The debugger
independently caught UIApplicationMain and validated UUID
19A87853-DD17-33B3-99DB-5C8AB6F0EC48 at base0x100800000. Saved calculator-main-v81.json.
After this observation, breakpoint16 was restored to the Notebook relaunch
observer and execution continued. The bounded CPU trap trace remains the source
for the next failure diagnosis. Viewer restarted with transcript
/tmp/live-view-v81-uart-5.txt; no Calculator UI has yet been verified.

The initial180-second trace ended without breakpoint/undefined exceptions.
Calculator81 subsequently exited with SIGTRAP sent by exc handler after127082
reported guest milliseconds. Its observed framebuffer was black except the
existing diagnostic banner (calculator-black-v81.png). Because trace coverage
ended before the failure was observed, this does not rule out a CPU trap.
A second trace uses a600-second upper bound and explicit clean-stop file so
coverage can span the next process's entire lifetime. No arithmetic is verified.

Second launch spawned Calculator86 and LS_OPEN_RESULT=1. The trace was enabled
before this launch and an independent host watcher is waiting for its exact PID's
launchd exit record before creating the clean-stop file.

A task/thread sample succeeded for PID86 and resumed all five threads. Its main
thread is in Swift generic metadata decoding, reached from SwiftUICore
ViewBodyAccessor.updateBody and AttributeGraph UpdateStack.update. This is later
startup progress than the V79 logger-initialization sample, not a proof of a
working scene. Symbols are in calculator-thread-symbols-v81.txt.

The temporary inspection-enable breakpoint27 unexpectedly kept firing despite
its one-shot setting when its callback returned false. It was explicitly deleted
before sampling. Restoration used a stopping one-shot breakpoint28, read back00,
and then independently read the byte as0x00 at the kernel stop. Execution resumed.
The brief breakpoint overhead makes this run unsuitable for latency measurement.

The full-lifetime trace caught exactly one user breakpoint before Calculator86's
SIGTRAP exit: ESR0xf2000001 (BRK#1), ELR0x18ace32f0, EL0->EL2. With cache slide
0x268c000 this is unslid0x1886572f0, SwiftUICore Image.Location.systemAssetManager
initializer+176. Disassembly shows a nil check branching directly to this BRK.
The preceding receiver is SFSCoreGlyphsBundle and the selector resolves to public.
The guest filesystem independently reports SFSymbols.framework and its
CoreGlyphs.bundle absent. Matching24A437 firmware contains both.

An isolated clone, service-root-glyphs-v82.dmg, now restores the matching complete
SFSymbols.framework resource directory:41 files,130271365 bytes. Every copied file
was SHA256-verified against its source before the image was cleanly detached.
The trustcache is unchanged (glyphs-v82.tc copies live-delta-v81.tc); no new
executable was introduced. This is a staged fix, not yet a runtime Calculator
success. V81 remains the only running Darwin guest, with SpringBoard resumed.
The full trace has stopped and QEMU exception logging is disabled.

An attempted saved-note export did not succeed: both the guest shell's glob and
/bin/ls were denied directory enumeration under /private/var/mobile/Containers/
Data/Application. /var/mobile/Documents did not contain Notebook.plist. No saved
file was modified or deleted. V81 is retained so the existing app container and
saved note remain available; migration to V82 is not yet established.
