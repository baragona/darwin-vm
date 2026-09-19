# Interactive iOS 27 demo

Target: the A19 / 24A437 guest, controlled from a live host window.

Completion requires a visible SpringBoard home screen and mouse input that
launches a real offline UIKit app. The app must show text and images, scroll,
and respond to buttons. A user must be able to type a note and draw; returning
home and reopening the app must preserve the content. Apple's Calculator
must launch and complete a calculation through input in the same guest.
Screenshots, input-monitor callbacks, and successful app registration alone
do not establish these outcomes.

V78 now verifies native tap -> visible counter update -> Home-key navigation
-> LaunchServices reopen with the counter retained. See
[evidence](evidence/home-navigation-v78.md). The app icon is missing from
SpringBoard and durable saving is not yet implemented.

The [Field Notes prototype](notebook-probe/README.md) runs in V79. Verified
framebuffers show text, labeled buttons, a bundled image, and a canvas. Native
tap and drag input drew a visible stroke; the app reported a successful atomic
file save. [Runtime evidence](evidence/notebook-runtime-v79.md) distinguishes
these checks from the still-pending text entry and scrolling tests. A fresh
process restored the saved drawing after Home and explicit termination. Calculator is installed and registered; its first launches trap before a usable
UI appears. The [launch investigation](evidence/calculator-runtime-v79.md) is ongoing.
V78 has been closed; V79 is the only running Darwin guest.

## Work remaining

- Stabilize compositing across unlock, app launch, home, and reopen. V78 exposed
  a QuartzCore push_surface assertion in the asynchronous layer renderer.
  Forcing CALayerHost setRendersAsynchronously: false is under runtime test.
- Replace one-shot capture and per-gesture helper startup with a persistent
  display/input transport suitable for mouse interaction. Measure latency;
  current UART diagnostic transfers are not a usable live frontend.
- Verify Field Notes text editing and scrolling. Initial rendering, drawing,
  and saved-drawing restoration after process termination are verified.
- Connect host keyboard and pointer events through the guest input system.
  Verify text entry and drawing, then Home and reopening without clearing data.
- Install/register the full Calculator bundle from this build's
  private/var/staged_system_apps, including required resources. The local
  inventory is recorded in evidence/calculator-inventory-v78.json. Its
  SwiftUI/CalculateUI dependencies require actual launch and interaction tests.

Keep Apple firmware, app binaries, and extracted assets local. Commit source,
reproduction notes, and appropriately scoped evidence to the research branch.
