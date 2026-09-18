# Status-bar rendering with Setup disabled (V72)

SpringBoard PID186 remains running with the temporary Setup-required-reason
setter override documented in setup-skip-runtime-v72.md. The previous goal turn
made progress by implementing that bypass; this follow-up verifies visible
rendering and narrows the remaining interaction failure.

## Thread sample

SpringBoard has nine threads in this sample. Its main thread is waiting through
CFRunLoopServiceMachPort (unslid0x18063646c) below UIApplicationMain
(unslid0x1849141cc). BackBoard PID52 has eleven threads. This single sample does
not show the earlier biometric worker buildup or prove sustained responsiveness.
The guest kernel inspection byte at 0xfffffe0017088db4 was changed from zero to
one only for sampling, then restored and read-verified zero. The completed
transcript is home-threads-v72.txt.

## Touch reaches SpringBoard; visible status bar

The virtual swipe completed with 14 matching monitor callbacks. A read-only
observer at -[SBLockScreenManager isUILocked], unslid0x224abaf04,
runtime0x2278f6f04, found self0x0600007992ff8380. Its byte at +0x190,
0x7992ff8510, was 1. The getter simply returns that byte.

The caller was runtime0x227a6ee54, unslid0x224c32e54:
-[SBControlCenterController grabberTongue:shouldReceiveTouch:] +76.
This verifies SpringBoard's touch filtering was reached, not that an unlock
gesture was recognized.

The subsequent completed display capture is no longer uniformly black. It shows
a white SEARCHING status label, signal dots, and a battery icon on a black
background. The image is home-swipe-capture-v72.png; its raw BGRA SHA256 is
 a03ac4b3e3f9f39a3c9054b9ffd833018077cb78362bfd3b70a7d8c63e3ddc43
and it has 2,624 colored pixels. It does not show a usable lock screen, app grid,
or app content. This sequence does not isolate touch from elapsed startup time
as the cause of the rendering change.

## Unlock observers

A repeated swipe completed with 14 matching callbacks. The observer at
unlockUIFromSource:withOptions: (runtime0x227a05c9c) had zero hits. A subsequent
read of the same lock-state byte still returned 1.

A Home action completed with two matching callbacks. Observers at
unlockWithRequest:completion: (runtime0x227a51d34) and
_unlockWithRequest:cancelPendingRequests:completion: (runtime0x227a50730)
each had zero hits. These observations cover only the specified methods and
input windows; they do not prove that every possible unlock path was inactive.
The UART collectors completed normally. All three temporary unlock observers
were removed; the lock getter observer was one-shot. No lock/authentication
result or lock-state byte was changed.

The Objective-C exception and failed-layout observers still each report two
hits from the previous launches, with no additional observed stops in this
experiment. The guest was left running. Setup bypass breakpoint21 remains active
alongside the existing VM bootstrap workarounds.

Next distinguish the missing cover-sheet/home gesture configuration from the
no-SEP key-store failures. Repeated AKS errors alone do not prove that the unlock
request was rejected: none of the monitored unlock entry points was reached.
