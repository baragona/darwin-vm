# Graphical iOS in QEMU: prior art

Reviewed 2026-09-18. Graphics and real apps in QEMU predate this fork.

- [Martijn de Vos, October 11, 2022](https://devos50.github.io/blog/2022/ipod-touch-qemu/): iPod touch 1G / iPhoneOS 1.0, with SpringBoard, graphical applications, display and multitouch. Includes demonstration and QEMU source links.
- [Axel Cohen / eShard, June 4, 2025](https://www.eshard.com/blog/emulating-ios-14-with-qemu-part2): iOS 14 / iPhone 11, SpringBoard, Settings and other apps, software rendering, multitouch via VNC, networking and IPA installation. This is the closest documented precedent for our current work.

The eShard report describes wrong-resolution UI artifacts, remaining Metal accesses in QuartzCore's software path, missing digitizer service identity, idle/backlight workarounds, and combining display and input VNC servers. These are useful investigation directions, not patches verified against our iOS 27 build. Their hardware multitouch stub differs from our guest virtual HID service.

Targeted searches for A19/iOS 27 graphical QEMU demonstrations did not find another comparable public result. That does not establish priority or exclude private work. Describe our contribution by its tested build and behavior: A19/iOS 27 diagnostic graphics and a real UIKit button interaction, building on darwin-vm. Do not claim the first graphical iOS QEMU emulator.
