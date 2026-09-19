# V81 changed-region display runtime

Booted service-root-live-delta-v81.dmg with live-delta-v81.tc (4422 entries).
The installed agent identity is recorded in live-delta-staging-v81.json.
V79 and V80 were closed through QMP; only V81 executes now. Obsolete LLDB
processes explicitly connected to already-closed Darwin guest ports were also
terminated. No unrelated VM was changed. Host memory pressure was substantial
before retiring the fallback; this is a possible latency contributor, not an
isolated performance measurement.

The boot collector correctly recognized the Bash5.3 prompt. UserManager PID16
sampling established shared-cache slide0x268c000 and base0x1023a0000. The persona
callback verified Mach-O magic and the seeded mobile UUID NSString. The launchd
callback verified base0x1006c4000. Kernel inspection was restored and read back
as zero before UI services started. The targeted extension-directory ownership
workaround is persistent from initial startup in this run.

A persona syscall-return breakpoint was armed too early and caught an ordinary
call while the registration command was being typed. The breakpoint was disabled
and execution resumed. The guest reported a truncated command-not-found, so the
registration command is being retried. This was debugger sequencing, not an
image boot failure. The return observer must stay disabled until the controlled
persona allocation is prepared.

Changed-region capture, keyboard delivery, and Calculator are not yet verified
in this guest. The broader goal remains active.

The initial helper commands used the legacy /bin/ls-registration-probe; the
verified newer helper is /bin/ls-open-probe. Its rebuild and a later direct query
stalled while SpringBoard44 was running. Both diagnostic clients were cancelled
with SIGINT to recover the shell. SIGTSTP did not return a prompt and is not
counted as a successful suspension.

Pausing only SpringBoard44 with SIGSTOP immediately allowed launchd list and
LaunchServices query to complete. The actual proxy reports Field Notes installed
at /Applications/Notebook.app. Thus lack of registration is not the demonstrated
blocker. A thread sample of paused SpringBoard44 found20 threads, including main
thread in Sleep.framework connection/retry code and a worker in CoreTelephony
connection/logging under SBTelephonyManager queue_setFastDormancySuspended. These
match recurring missing sleepd/commcenter service messages; a single sample does
not establish which thread causes the contention. Kernel inspection was enabled
for this sample and must be restored to0 before proceeding.

Kernel inspection was subsequently restored and read back as0. SpringBoard was
resumed with SIGCONT. A fresh open request reached Notebook's posix_spawn before
the proposed Sleep retry hook was installed; that hook remains unarmed and is
not credited with progress. Persona1003 was absent (query errno3), allocated as
type2/UID501 (return0), and independently queried again (return0). The controlled
experiment restored and verified every saved register and scratch byte, then
deleted its spawn and syscall-return breakpoints. Notebook entered
UIApplicationMain at base0x102be8000; the app log observer is at base+0x5e00.

The protocol2 viewer has now been started against V81. App launch and transport
startup are not yet proof of a rendered, responsive, editable scene.

The agent emitted LIVE_INPUT_READY2. Full frame1 was rejected after console
interference; full frame2 validated and showed the real lock screen. Subsequent
patches referenced their exact previous frame and validated, including an
11,920-byte raw Home-indicator rectangle compressed to280 and229 bytes.
LIVE_FRAME_SAME confirmations followed. This is actual guest proof of patch
application and full-frame recovery, not just host unit tests. The first60-second
observation window included initial acquisition; settled latency requires its
own measurement. Notebook reported LAUNCHED and SCENE_VISIBLE_WITH_IMAGE, but
the visible display remained covered by the lock screen at this checkpoint.

An isolated headless Chrome session loaded the actual viewer page. Its displayed
image reported832x1808 and a395.75x860 browser bounding box. Puppeteer mouse
events performed a14-sample upward drag through the page's pointer handlers.
After queued capture/input completed, validated frame19 showed Field Notes with
its heading, buttons, note text, bundled image, and drawing canvas. A screenshot
of the browser page independently captures the frontend displaying that scene.
This verifies actual browser mouse -> guest unlock -> live rendered app, beyond
the earlier HTTP-only input test. The app had been launched through LaunchServices
before unlocking; clicking its Home icon is still a separate required check.

Browser pointer clicks on the editor and Draw button and a physical KeyX event
were sent through the page handlers. Browser inspection confirmed focus on the
image and empty browser output queues, and request logging confirmed normalized
D/U commands at the expected button coordinates. Guest command acknowledgements
alone did not translate into a verified caret, button action, or changed text.
Frame19 remained the visible app scene with the original note. The next input
diagnostic should compare the persistent endpoint against the previously working
one-shot helper in this same running guest; typing is not yet established.

Only V81 remains running. The local viewer uses protocol2 on127.0.0.1:8989; an
isolated headless Chrome session is exercising the real page. The proposed
Sleep retry hook remains unarmed. Persona experiment observers were deleted.
Kernel inspection was read back as0 after the thread sample.

Later frame20 showed a caret at the end of the note, and the read-only app observer
reported Unsaved changes. The captured text still did not visibly contain the
test character, so keyboard text delivery remains unverified. This delayed visual
response also warrants comparing the persistent renderer against the existing
display-capture-probe --wake path before attributing the issue solely to HID.
The persistent renderer currently reuses its IOSurface and does not request an
explicit display-state transition; the old capture helper does when given --wake.

A separate25-second settled observation collected50 viewer states. Last validated
capture age ranged from0.063 to2.648 seconds, with2 rejected frames total and
a final age of0.140 seconds. These figures include unchanged-frame confirmations
and do not establish input-to-visible-change latency. Large transitions still
require much larger patches and can be substantially slower.
