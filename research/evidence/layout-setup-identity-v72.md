# Identifying the missing transition layout (V72)

Read-only inspection of the saved exception stack identifies the failed target as
an SBAppLayout containing **com.apple.purplebuddy (Setup)**. The coordinator's
appLayouts array is an __NSArrayM with count **zero**. This narrows the failure
from an unknown transition to Setup layout registration. It does not establish
why the array is empty or that inserting the layout would be correct.

## Recovering registers without debugger unwind metadata

The remote debugger could not report caller general-purpose registers. They can
still be recovered from the actual callee prologue and saved stack memory:

- Foundation assertion-handler entry, unslid `0x180f04820`, subtracts 0x70 from
  SP and saves x24/x23 at SP+0x30/+0x38. Its FP is SP+0x60.
- At the throw, FP is `0x16b7b8d80`; SP is `0x16b7b8d20`. The saved x23 at
  `0x16b7b8d58` is `0x0900007c84f66b50`. The handler's own x23 has since changed;
  reading that live register would give the wrong target.
- The caller places the result of `appLayout` in x23 at unslid `0x224b12814`,
  and uses it as the containsObject argument at `0x224b12928`. The selector
  stub's runtime string at `0x1f82bdd80` reads `appLayout`.
- Target object class metadata resolves to the string `SBAppLayout` at
  `0x1fdddf126`. Its allItems slot (+0x60) points to the single-item container
  `0x7c8574d5f0`, holding display item `0x7c7bc111c0`. The item's bundleIdentifier
  slot (+0x30) is CFString `0x1ebba5e90`, whose byte pointer `0x1837e755f` reads
  `com.apple.purplebuddy`. The target layout also contains this identifier at
  +0x18. No Objective-C methods were invoked during these reads.

## Verifying the empty array

The transition caller's saved FP is `0x16b7b9420`. Its prologue puts local SP at
FP-0x690, or `0x16b7b8d90`. The coordinator stored at SP+0x88 is
`0x0700007c8462de00`; its +0x40 array is `0x0300007c7ada5ad0`.
Class metadata resolves to `__NSArrayM` at string `0x1fd9af2e5`.

Disassembly of `-[__NSArrayM count]`, unslid `0x180620cb8`, reads the dynamic
storage offset at unslid `0x1e3f2310c`. Runtime `0x1e6d5f10c` contains 0x10.
The count field is storage+0x14, hence object+0x24. A 32-bit read at
`0x7c7ada5af4` returned zero. This uses the actual firmware representation rather
than assuming the array's first word is its count.

## Guest state and next investigation

No guest memory or registers were changed by this inspection. Continuing the
uncaught exception terminated SpringBoard: launch-probe reports PID=-1,
LAST_EXIT=6. The UART command completed with its marker. BackBoard, RunningBoard,
MobileGestaltHelper, budd, biometrickitd, and the guest shell remain running.
The stderr transcript confirms the same assertion but gives no additional Setup
launch error. SpringBoard has not yet been restarted after this failure.

Setup's LaunchServices registration was already verified in V72. That does not
prove its process or scene launched. Next inspect Setup process/scene creation
and the coordinator's layout insertion path; do not mask the consistency check
or manually add a layout without understanding the missing lifecycle step.

The exact recovered pointers and terminal service state are in
`layout-setup-identity-v72.json`; guest stderr and job list are in
`layout-setup-stderr-v72.txt`. Addresses are specific to this boot and process.
