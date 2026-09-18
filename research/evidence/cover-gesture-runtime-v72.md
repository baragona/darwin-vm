# Cover-sheet gesture acceptance and edge-position test (V72)

SpringBoard PID186 remains under the Setup-required-reason override. This follows
the status-bar-only rendering observation in home-lock-runtime-v72.md.

## Touch acceptance

During a normal virtual swipe, a one-shot observer stopped at
-[SBCoverSheetSystemGesturesDelegate gestureRecognizer:shouldReceiveTouch:],
unslid0x224b79670, runtime0x2279b5670. The arguments were:

- delegate: 0x0900007992ff3250
- recognizer: 0x0900007992471180
- touch: 0x0e000079899b5500
- return address: 0x187c837fc

The recognizer equals the delegate's +0x50 slot. For this case the method calls
extendSwipeUpRegion on the object in delegate+0x60 (0x0b000079932c5fe0).
A one-shot observer at the return address read w0=1. Thus the initial touch was
accepted by this delegate. No acceptance result was overridden.

A separate observer at gestureRecognizerShouldBegin:, unslid0x224ba1ef0,
runtime0x2279ddef0, had zero hits across the normal and edge-position swipes.
This only covers that delegate method, not every gesture recognizer or every
possible path to recognition. The recognizer's class/state and configured
regions remain to be inspected.

## Starting closer to the bottom edge

The unmodified probe starts at normalized y0.96, about19.84 pixels above the
bottom of the 496-pixel display. Test y0.995 (about2.48 pixels above the bottom)
for just the first hand and finger events. Remaining coordinates and timing
were unchanged. This tests the starting position; it does not produce a uniformly
rescaled trajectory or test alternate gesture durations.

One-shot observers at IOHIDEventCreateDigitizerEvent (runtime0x191edd6ec) and
IOHIDEventCreateDigitizerFingerEvent (runtime0x191edd6b8) were conditional on
x1==0 (preallocation timestamp) and lr<0x110000000 (the probe executable caller).
The actual return addresses were0x100cc0cec and0x100cc0d4c. At each stop,
d0 was0.5 and d1 was0.96. Only d1 was changed to0.995, with read-back verification.
The probe preallocates every event before dispatch, so the debugger pauses did
not interrupt this gesture's dispatch sequence.

The guest log confirms first-frame Y=0.995, successful dispatch, all14 matching
monitor callbacks, HID_PROBE_END, and the completion marker. The unchanged swipe
also completed normally. The should-begin observer still had zero hits.
No new post-test screenshot was taken; this experiment cannot claim a visual
improvement or prove that the closer starting point was accepted by the same
recognizer, because the touch-acceptance observer was one-shot for the earlier
normal swipe.

All new observers were one-shot or removed. There is no persistent input override
or probe-source change. Guest execution was resumed. Setup bypass breakpoint21
and the existing bootstrap workarounds remain active. No host changes were made.

Next inspect this recognizer's class, state transitions, touch history, and
configured edge region, or the home-gesture path that receives its events. Do
not infer an authentication rejection from key-store errors alone when the
monitored gesture-begin and unlock methods have not been reached.
