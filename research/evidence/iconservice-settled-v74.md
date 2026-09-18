# Settings artwork eventually reaches the live SpringBoard display

A later V74 capture now visibly contains the Settings gear in both the small
icon at the upper left and the large partial card at the right. This changes the
next investigation: artwork is reaching the live display, while size/layout and
application launch remain unresolved.

## Current service state and sample

iconservice-current-v74.txt completed its UART marker. IconServices remained
PID51, SpringBoard89 and LaunchServices65. Reading the icon-service stdout and
stderr produced no diagnostic payload or missing-file error in that transcript.

A fresh thread sample in iconservice-resample-v74.txt found two IconServices
threads, rather than the earlier three-thread sample with a metadata worker:

- main: runtime0x240a101e4 / unslid0x237efc1e4,
  __sigsuspend_nocancel; caller0x1b7617f44 /0x1aeb03f44,
  _dispatch_sigsuspend
- worker: runtime0x240a0ba80 /0x237ef7a80, __workq_kernreturn;
  caller0x18904e910 /0x18053a910, _start_wqthread

LaunchServices65 had two threads with its main thread in CFRunLoopRun and
_runServerMainRunLoop (runtime0x18fa5da0c / unslid0x186f49a0c), plus a workqueue
wait. Both task samples returned RESUME_RESULT=0. No icon-generation metadata
wait appears in this later snapshot. It cannot establish when or how the prior
request finished.

The guest inspection pointer and byte were verified at a kernel stop, temporarily
enabled for these samples, then restored and read back0 at0xfffffe0017088db4.
The one-shot kernel observers were consumed; the guest was resumed.

## Visible result and pixel comparison

The wake/capture/export command completed, with display state on, render result1,
206336 changed pixels,206336 opaque pixels, and825344 output bytes. Decoding the
zlib stream and exact frame length succeeded. Current frame SHA256:
a1b654c5c2b139967476645326baedb733c0a8fed45e47befd17cdea8ed4efd0.

Compared with iconservice-visible-v74.png, exactly6663 pixels differ:
744 in the small-icon region and5919 in the right-card region. The overall
changed bounds are[x40,y36,x416,y142), and those two regions account for all
differing pixels. See iconservice-settled-diff-v74.json. The new PNG was visually
inspected: both formerly plain areas now show the Settings gear. The rest of the
captured layout is unchanged.

This is live SpringBoard display evidence, distinct from the V75/V76 isolated
local compositor tests. V75/V76 ran as separate guests and were stopped after
testing; they did not replace V74's binaries or inject their images into it.
In V74 this follow-up only read service state, sampled threads and woke/captured
the display. No image pointer, icon scale or renderer result was substituted.

## Interpretation and limits

Restoring IconServices alone had produced no immediate visible change in the
first capture. By this later capture, artwork is present. Elapsed processing,
metadata availability and fresh display activity may be relevant, but these
observations do not isolate the exact trigger or latency. Do not infer that the
unrelated local-render tests fixed SpringBoard.

The blanket claim that only shapes render is no longer true for the current
guest. The remaining obvious visual defects are the tiny Settings presentation,
oversized status text, and large white/partial cards. The earlier quarter-scale
requests are still a clue, not a proven persistent transform of the visible
view. Next identify its current view hierarchy/transform and presentation state,
and continue testing launch and input. Neither a usable launcher nor a working
Settings launch is established by this artwork result.
