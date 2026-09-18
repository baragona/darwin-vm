# Registered virtual touchscreen (V67, 24A437)

Adds --virtual-swipe using HIDVirtualEventService. A dynamically registered
NSObject delegate exposes a touchscreen usage pair (page 13, usage 4),
DisplayIntegrated/Built-In flags, and diagnostic product/vendor metadata.
Every property query is logged; unsupported event-copy/output requests return
nil/false. The delegate waits for enumeration notification 10, reads the
framework's assigned serviceID, then dispatches through dispatchEvent:.
Termination notification 11 prevents further dispatch. The bounded probe
cancels its service on normal completion.

API reference: [Apple HIDVirtualEventService source](https://github.com/apple-oss-distributions/IOHIDFamily/blob/777ccd9698845aadf711e32d843c8c9b777431d9/HID/HIDVirtualEventService.m)
and [delegate protocol](https://github.com/apple-oss-distributions/IOHIDFamily/blob/777ccd9698845aadf711e32d843c8c9b777431d9/HID/PrivateHeaders/HIDVirtualEventService.h).
This experiment does not assume that enumeration implies BackBoard touch
routing; those need separate runtime observations.

Compile both sources, then sign with the existing event-monitor and
event-dispatch entitlements:

```sh
xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib \
  research/hid-input-probe.c research/virtual-touch-service.c \
  /tmp/a19-restore/usr/lib/libSystem.B.dylib -o /tmp/a19-hid-input-probe-v67
codesign -s - --entitlements research/hid-input-entitlements.plist /tmp/a19-hid-input-probe-v67
codesign --verify --strict /tmp/a19-hid-input-probe-v67
```

Warnings-as-errors compilation and strict signature verification passed.
[Installed binary/source hashes and trust-cache addition](virtual-touch-image-v67.json).
V66's CFNumber registry-ID source fix is included in this installed binary.
## Runtime: protocol metadata is optional

An unchanged first run exited 1 before creating a service:
[early transcript](virtual-touch-early-v67.txt). A breakpoint on
objc_getProtocol (unslid 0x1804275c0, slide 0xdd2c000) confirmed the requested
name was HIDVirtualEventServiceDelegate and its return was NULL. Dynamic
class allocation had succeeded (0x78cf00c030). The probe's strict protocol
check, rather than an OS entitlement rejection, caused this failure.

For the next diagnostic run, PC was moved from the return site 0x1005d1690
to 0x1005d16cc, skipping only that probe-local check/adoption. This does not
change the HID server or its permission decisions. The service initialized,
received notification 10, and acquired ID 0x7938d040000000. All 14 dispatches
returned 1 and all 14 matching monitor callbacks arrived; process exited 0.
[Protocol experiment](virtual-touch-protocol-v67.txt).

## BackBoard resolves the registered touchscreen

A second run repeated the same probe-local workaround at ASLR addresses
0x1041fd690 → 0x1041fd6cc. The direct-touch observer was
0x2389bce88 (unslid 0x22ac90e88 + slide), immediately after service lookup.
All **14 hits returned x0=0x0800007938d01180**, a non-NULL service. The event
sender inspected on the first hit was 0x007938d042000001, matching this
run's assigned virtual service ID. The first hit paused for inspection;
remaining hits auto-continued. No BackBoard service result was overridden.
Breakpoint hit count was read back as 14, then the observer was removed.
[Routing transcript](virtual-touch-routing-v67.txt).

The service received DeviceOpenedByEventSystem and TouchDetectionMode
property changes. Requests for displayUUID returned nil; this did not
prevent service resolution in this run. This does not establish correct
screen coordinates or delivery to SpringBoard.

These tests ran against the initial BackBoard process **before SpringBoard
startup**. They establish registration, monitor delivery, and BackBoard
service resolution, not successful welcome-screen navigation. The temporary
kernel inspection byte used to derive the slide was restored and read back
as zero before the input experiments and again before shutdown.

The source now logs protocol metadata presence and only adopts the protocol
when metadata exists. That corrected source is a separate V68 build; it is
not the binary represented by the V67 source hashes. It passed warnings-as-
errors compilation and strict signature verification before installation.
[Corrected installed identity](virtual-touch-image-v68.json).


## V68: corrected source works without a debugger

The installed corrected binary booted with no debugger attached and no
inspection-byte change. It logged protocol metadata absent, then received
notification 10 and service ID 0x1017628f00000. All 14 virtual dispatches
returned 1, the monitor returned all 14 matching frames, and the process
exited zero. [Complete clean-run transcript](virtual-touch-clean-v68.txt),
[machine-checked counts](virtual-touch-validation-v68.json).

This clean run verifies registration and monitor delivery. BackBoard's
non-NULL service result was directly observed in V67; it was not inspected
in V68. SpringBoard remains unstarted in this boot, ready for the next
end-to-end welcome-screen gesture experiment.
