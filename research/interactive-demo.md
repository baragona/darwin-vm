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

The [Field Notes prototype](notebook-probe/README.md) now builds and is staged
in a separate V79 image alongside Calculator and the configurable drag helper.
Its editor, image, scrolling, drawing, and atomic local saving are implemented
but not yet runtime-verified. V79 has now booted and LaunchServices independently reports both bundles
installed. Its LCD reports 832x1808. The V78 baseline is paused as a fallback;
notebook rendering and Calculator interaction remain unverified.

## Work remaining

- Stabilize compositing across unlock, app launch, home, and reopen. V78 exposed
  a QuartzCore push_surface assertion in the asynchronous layer renderer.
  Forcing CALayerHost setRendersAsynchronously: false is under runtime test.
- Replace one-shot capture and per-gesture helper startup with a persistent
  display/input transport suitable for mouse interaction. Measure latency;
  current UART diagnostic transfers are not a usable live frontend.
- Run Field Notes in the guest and verify its editor, image, scrolling,
  drawing and explicit local saving. Verify the app Documents directory is
  writable instead of using the denied global Logs path.
- Connect host keyboard and pointer events through the guest input system.
  Verify text entry and drawing, then Home and reopening without clearing data.
- Install/register the full Calculator bundle from this build's
  private/var/staged_system_apps, including required resources. The local
  inventory is recorded in evidence/calculator-inventory-v78.json. Its
  SwiftUI/CalculateUI dependencies require actual launch and interaction tests.

Keep Apple firmware, app binaries, and extracted assets local. Commit source,
reproduction notes, and appropriately scoped evidence to the research branch.
