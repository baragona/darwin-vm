# Virtual touchscreen display association (V68, 24A437)

The registered virtual touchscreen reaches BackBoard and creates a contact,
but its missing `displayUUID` causes zero display geometry and NaN touch
coordinates. Supplying the actual LCD identifier restores the geometry
lookup and reaches the observed event-posting method. The captured screen
still shows Hello; successful welcome-screen navigation is not established.

## Startup and baseline

The installed binary is unchanged from [V68's manifest](virtual-touch-image-v68.json).
Cache slide was derived as 0x2b38000. UserManager PID 39 had base
0x100ddc000; its borrowed mobile NSString 0x78db01c230 was checked against
CE7B39AA-4774-5BA4-BD1C-A29AB22DB754. The other key was the all-F UUID.
The temporary inspection byte 0xfffffe0017088db4 was restored and read back
zero before support-service startup and again after the experiments.
[Boot-specific startup commands](virtual-touch-startup-v68.lldb).

System app rebuild returned 1. Launchd base 0x104efc000 was derived from
LR 0x104f1621c and checked for Mach-O magic. The extensionkitservice check
reported the original ExtensionFoundation XPC path, helper result 1,
directory mode 0755, and UID/GID 99. A one-time stat UID write at
0x7d15524570 allowed launch; that breakpoint was removed. SpringBoard was
PID 60 and CommCenter PID 66. Existing display, persona, credential, and
backlight startup workarounds remain necessary.

The first wake left capture prefill unchanged. A later capture reported on
but was all black; after Home down/up and another wake, the frame showed the
status bar and home indicator. The first registered swipe then captured
[Hello](virtual-touch-ui-v68.png), with all 14 dispatches returning 1 and
14 matching monitor callbacks, process exit 0.
[UI transcript](virtual-touch-ui-v68.txt).

## Registration and contact creation work

Read-only observers during [the repeat run](virtual-touch-path-v68.txt):

| Stage (unslid address) | Hits | Observation |
| --- | ---: | --- |
| Direct-touch service lookup, 0x22ac90e88 | 14 | x0=0x0100007455192b20, non-NULL |
| Handler block's display lookup result, 0x22ac91dd8 | 14 | x0=0; fallback continues |
| handlePathCollectionEvent:, 0x22ac91f98 | 14 | state x0=0x0300007455080300, event non-NULL |

The following [contact run](virtual-touch-contact-v68.txt) observed the
path-kind check at 0x22ac920bc returning zero for all 14 events, selecting
the traditional path. `_newContactForPathIndex:pathEvent:` at 0x22ac8fa7c
was reached once. No result overrides were applied in these runs.

## Zero geometry produces NaN coordinates

At `_denormalizedLocationForNormalPoint:offset:` + 0x88, unslid
0x22ac97524 (runtime 0x22d7cf524), the geometry at sp+0x40 was:

```
width=0 height=0 scale=0 reserved=0
normalized-origin=(0,0) normalized-size=(1,1)
```

The first input was (0.5, 0.96). At the method epilogue, 0x22ac975f8,
all six coordinate doubles at output+8 were **NaN**. The observer then
recorded the same zero geometry for the remaining frames (14 hits total).
The selector read from the exact-build stub was `geometryForDisplayUUID:`.
The touch state's display identifier at state+0x10 was NULL.
[Hit-test transcript](virtual-touch-hit-test-v68.txt).

The `_postEvent:attributes:toDestination:initialTimestamp:` observer at
0x22ac9c650 had zero hits. This is evidence about that method, not a claim
that all possible event-delivery paths were exhaustively instrumented.

A diagnostic [geometry-only run](virtual-touch-geometry-v68.txt) wrote
416, 496, and 1 as doubles at sp+0x40, +0x48, and +0x50 for all 14 frames.
The first converted coordinate became (208,476), the last (208,99), with
unrounded values (208,476.16) and (208,99.2). The posting observer remained
at zero hits and the frame remained Hello. Those temporary stack writes
were removed before the display-association test.

## Associating the actual LCD restores the lookup

The BackBoard process's CADisplay main-display global at
0x1e7025308 + slide yielded object 0x0e0000010525abb0. Its implementation
pointer at object+8 was 0x0500007455109180; implementation+0x80 held the
uniqueId string 0x0a00007454d47680. Its complete inline text was:

```
19F96189-BCA5-42CD-883F-6EBF71944977
```

This value was discovered in this boot; it must not be hard-coded for
future guests. Exact-build `CADisplay uniqueId` and `UUID` both read the
implementation's +0x80 member.

At `_queue_displayUUIDForDigitizerService:` + 0x24 (0x22ac8c278), immediately
before retaining the service's returned display identifier, a conditional
breakpoint replaced only NULL x0 with that borrowed LCD string. The normal
retain/autorelease path remained intact. The first exploratory activation
of this override occurred partway through the display-ID inspection run;
that run is not a clean gesture test.

The subsequent [complete associated run](virtual-touch-associated-v68.txt)
used this identifier override from service registration onward, with no
geometry override. All **28 geometry observations** (two per frame) read
416,496,1,0,0,0,1,1. The previously silent event-posting method was reached
**twice**, with non-NULL event, attributes, and destination arguments.
All 14 dispatches succeeded and all 14 monitor callbacks matched; exit 0.
The display-ID override had 64 total hits across the exploratory and full
runs, so that count must not be attributed solely to the final gesture.
All input observers and the identifier override were removed afterward.

The final frame remains byte-identical to Hello after the original swipe:
SHA256 1de1a87a020bf5619ba529795d97cbee133f80e75378115682cfec777d37727b.
[Associated capture identity](virtual-touch-associated-v68.json).
No successful UI navigation is claimed.

## Source change and next test

`virtual-touch-service.c` now loads QuartzCore, gets CADisplay.mainDisplay's
uniqueId, and adds it as `displayUUID` before activating the virtual service.
A missing display identifier fails explicitly. This avoids a hard-coded
UUID and preserves normal BackBoard geometry lookup.

The V69 source compiles with warnings-as-errors and passes strict signature
verification. [Build identity](virtual-touch-display-build-v69.json).
It is **not yet installed or runtime-tested**. The next test must install
it, verify the property callback supplies displayUUID and that geometry is
valid without the identifier override, then investigate delivery/gesture
handling if the welcome screen still does not advance. The V68 debugger
experiment supports this change but does not substitute for that test.
