# Larger virtual display fixes the tiny home-screen presentation

The V74 guest now has an832x1808 LCD, replacing the diagnostic416x496 mode.
The captured home screen has a substantially larger Settings icon and label,
with no adjacent App Library card intruding from the right. The status/control
proportions in the preceding lock-screen capture are also improved. This is
concrete progress on the size mismatch; materials, icon delivery after restart,
app launching and complete input behavior remain unfinished.

## Scoped geometry experiment

The earlier hierarchy established a208x248-point home-screen window inside the
416x496-pixel display. To test geometry without forcing icon scale, disassembly
of CAWindowServerVirtualDisplay.initWithOptions: was used to find the decoded
width and height immediately before display construction. At unslid0x184767978
(runtime0x18d27b978), x22 was416 and x0 was496. A conditional one-shot callback
changed those registers to832 and1808. No shared NSNumber values were changed.
See tall-display-v74.lldb for the historical address and callback.

The established virtual-display bootstrap flag and LCD-name overrides were also
armed for the new BackBoard process. The first serial restart attempt was
truncated while the debugger remained in its interactive command editor; its
fragment did not execute the requested restart. The shell was recovered and
GUEST_READY verified before the clean retry. Guest services were then stopped
and BackBoard started. SpringBoard was started again by service demand.

`display-probe` independently reported:

```
CADISPLAY_ENTRY=0 ID=1 NAME=LCD EXTERNAL=0 SUPPORTED=1
CADISPLAY_ENTRY=0 CURRENT_MODE=<CADisplayMode 832 x 1808 internal_panel (fixed)>
```

The display UUID is now CBD48739-C000-42D5-82D6-6CD7EA1B26FE. Touch probes
rediscovered it; the prior display UUID and prior SpringBoard object addresses
must not be reused. A direct read of the new logical home-screen window bounds
was attempted through a getter observer, but it received no hits and was
removed. The anticipated416x904 logical size is therefore not yet directly
verified; the physical mode and rendered geometry are verified.

## Restoring the known startup workaround

Fresh SpringBoard processes initially aborted after launchd rejected the original
ExtensionFoundation XPC bundle. At launchd0x102099d0c, base0x102070000 was verified
by Mach-O magic. The first observed path was PassKitCoreXPCService, which was
left unchanged. A callback then matched only the exact extensionkitservice.xpc
path, helper result1, directory mode040755 and UID99. It changed only the
returned stat UID at0x751940c6b0 to0; WriteMemory returned4 bytes and success.
The callback disabled itself and was later removed. SpringBoard146 then remained
running with BackBoard134. IconServices51, LaunchServices65, UserManager54 and
other support services remained in the same guest. Existing Setup/authentication
bootstrap overrides remain necessary; no real SEP/keybag unlock is claimed.

## Captures and limitations

Early captures were display-off placeholders with zero changed pixels. One later
capture hit its30-second alarm before completion. These are not valid UI images.
After startup settled, two wake/capture runs reported display-on and completed
full6017024-byte frames. The UART observation timeout occurred before the final
job-list marker; the VM's serial logfile independently recorded that marker,
preserved in tall-display-wake-completion-v74.txt. No guest restart was inferred
from the observation timeout.

The completed frame was exported and decoded at832x1808, SHA256
6153a45c73f788e062f0d5023d1824bdb1e5a93c6eb0f3c1c87a8e56e80e7d28.
Visual inspection of tall-display-v74.png shows the lock-screen swipe prompt,
status bar, bottom controls and home indicator. The previous swipe had occurred
while the display was off.

A fresh swipe with the display on received14 matching callbacks and reached the
home-screen capture, tall-display-home-v74.png. All1504256 pixels are opaque;
192770 are colored. Exact zlib/frame validation passed. Frame SHA256:
02997a1eb162d800731ed3aec430d98128ceee116900a19948ff3a225a588742.
The icon and label are now legible at a much larger size. The icon is still a
placeholder in this immediate post-restart frame, and the search/dock materials
are white. The earlier smaller-mode guest did eventually receive gear artwork;
its delivery at the larger size remains to be observed. No Settings launch was
verified by this display experiment.

The larger mode remains active in the running guest. The size, flag, and LCD-name
breakpoints were one-shot and consumed. The stat and home-window observers were
removed. This is a runtime experiment, not a persistent firmware patch or an
upstream-ready general display-configuration implementation.
