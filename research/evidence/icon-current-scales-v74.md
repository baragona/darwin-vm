# Settings icon content scale persists at one quarter

A read-only _applyIconContentScale: observer at runtime0x1cdb006f0 received no
hits during a fresh Home down/up input. icon-scale-home-v74.txt completed with
two matching HID callbacks and a display-on capture at416x496. These monitor
callbacks do not establish that SpringBoard recognized Home. The observer was
removed; no scale argument was modified.

A second read-only observer at runtime0x18914a46c, a CoreFoundation run-loop
return point, inspected the two previously identified Settings views when their
memory was mapped. It only accepted a view if its _icon field at+0x1a0 referred
to the previously verified Settings model0x7d4b4055c0 (after stripping the top
byte). Both matched. It read the scale field using the runtime ivar offset at
0x1ee92b784, not an assumed offset. Current values:

| View | iconContentScale | Scaling enabled | Forcing enabled |
| --- | --- | --- | --- |
| 0x7d4b5f3000 | 0.24999999999999967 | true | false |
| 0x7d4b6a6a00 | 0.95 | true | false |

The scaling flags are packed bits. The initial diagnostic printed raw bytes71
and117; these are not BOOL values. Disassembly of isIconContentScalingEnabled
at unslid0x1c4fbac18 tests bit6, which is set in both. The forcing getter at
0x1c4fbac38 reads bit7, clear in both. The JSON preserves raw bytes and decoded
flags. The observer disabled itself after the successful read, was then deleted,
and the guest was resumed. No guest memory was written in this inspection.

Static tracing also resolves the two scale factors in _updateIconContentScale
(unslid0x1c4fe9610): it uses iconContentScale when content scaling is enabled,
and conditionally multiplies it by _additionalLiftScale before dispatching
_applyIconContentScale:. The iconContentScale setter at0x1c4f933c0 stores its
argument and dispatches _updateIconContentScale. The selectors were read from
the exact firmware's dispatch stubs and string data.

This establishes a persistent quarter-scale field on a real Settings view,
beyond the earlier argument-only observations. It does not yet establish which
view supplies each visible icon or the full ancestor/presentation transform.
Next inspect view hierarchy/geometry or capture the setter caller during a fresh
layout. The virtual display remains416x496; its effect on layout is a hypothesis,
not a verified cause. No scale override or resolution change was applied.
