# ExclaveOS indicator assets runtime test (V62, 24A437)

Image includes the [17 original ExclaveOS asset files](exclave-indicator-assets-v62.json)
and [staged original CommCenter](commcenter-image-v62.json). CommCenter was
not launched during the initial indicator test.

Cache slide 0x1f908000; UserManager PID 38 base 0x104628000. Verified mobile
NSString 0x7be900c8c0 from its UUID contents. The developer inspection byte
was restored/read back zero before startup services.
[Startup debugger snapshot](exclave-startup-v62.lldb).
System app rebuild returned true, main virtual LCD is 416x496.

Launchd base 0x10283c000 was derived from LR 0x10285621c and verified by
Mach-O magic. Original extension path passed the framework check, with mode
0755 and UID/GID 99:99. One stat UID override at 0x77ea9c8bb0 allowed launch;
the breakpoint was deleted. SpringBoard PID 59, backboard PID 51.

## Original manifest and Camera lookup succeed

At runtime 0x2be46d1ec, the manifest directory returned x0=0xd000000000000073,
x1=0x80000002be4a9ce0. Its 115-byte string at 0x2be4a9d00 names the preferred
/private/preboot/Cryptexes/ExclaveOS/System/ExclaveKit/System/Library/PrivateFrameworks/SILManagerAssets.framework/
directory. The breakpoint was removed without changing those registers.

At runtime 0x1a3e77e78, immediately after the original SILManager name lookup,
w0=0 for x19=0x2089210c0. The CFString's six bytes at 0x1a411c37f are Camera.
This is the expected original result, with no fallback, branch change, or
return-value substitution. The observation breakpoint was removed and the
first commit continued. This fixes the prior Invalid indicator name Camera
failure; usable display output still requires independent capture evidence.


## Capture and telephony follow-up

[Capture before CommCenter](exclave-display-capture-v62.txt) returns API 1,
writes 825,344 bytes, and changes all 206,336 active pixels. Nonzero RGB count
and opaque count are both zero. This is still a black capture, not usable UI.
SpringBoard PID 59 remains alive with LAST_EXIT=0.

Created /var/wireless with UID/GID 25:25, submitted the staged job, and then
explicitly started com.apple.CommCenter. Submission alone left PID=-1.
[Explicit start](commcenter-launch-v62.txt) shows original executable launch,
PID 68, and successful launchd running state. Standard error was empty at
the observed reads; dependent-service warnings remain in UART.

[Thread samples](commcenter-threads-v62.txt) successfully inspected and resumed
both processes. [SpringBoard main-thread symbols](springboard-after-commcenter-v62.json)
show CFRunLoopServiceMachPort under CFRunLoopRun, GSEventRunModal,
UIApplicationMain, and SBSystemAppMain. Unlike the V60 sample, this one is
waiting in the normal event loop, not a synchronous telephony query. This
single sample does not prove all CommCenter services are functional or that
its launch alone caused the change. CommCenter is still initializing in its
own sampled stack.

The temporary developer inspection byte was enabled/read back 1 in kernel
context, then restored/read back 0 in kernel context. An intervening restore
attempt from userspace failed and was retried successfully.

[Capture after CommCenter](commcenter-display-capture-v62.txt) has the same
pixel counts: all changed, RGB all black, none opaque. Next investigation is
display state and scene presentation. No usable graphical output yet.

## Display-state follow-up prepared for V63

The LCD-only displayState getter breakpoint fired once, but its state byte
was not sampled before continuing. It therefore provides no evidence that
the LCD was on or off. A later observation did not catch another call; that
breakpoint was removed. Read the exact build's cadisplay_state_to_string table
(unslid 0x1e05d2960, runtime 0x1ffeda960): 0 off, 1 on, 2 flipbook,
3 suppressed. Getter implementation is unslid 0x1845c559c; display ID is at
CADisplayStateControl+0x28 and its shared state pointer at +0x30.

Extended display-capture-probe to report the actual state. Default invocation
only observes; explicit --wake calls transitionToDisplayState:withCompletion:
with state 1 and then services the default run loop for up to two seconds.
It reports the subsequent state and still checks actual captured pixels.
The request returning or the wait finishing is not a successful wake/render
criterion. No HID input is supplied by this option.

The updated source compiles with warnings-as-errors, and its ad-hoc signature
passes strict verification. Installed and byte-verified it in the stopped
image; added its CDHash to trust cache v1 (4,401 -> 4,402), preserving every
prior entry and UUID. [Build and image identities](display-state-probe-v63.json).
This extension is not yet guest-runtime tested.

All five remaining debugger breakpoints were removed, LLDB detached, and
QEMU exited before mounting the image. Image detached successfully after the
probe update. Guest is stopped. Next: boot V63, capture without --wake, then
compare with an explicit --wake request if the state/output warrants it.
