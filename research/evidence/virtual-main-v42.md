# Built-in virtual main display experiment (24A437, v42)

No shared-cache code/signatures or host security settings were changed.
QEMU hardware breakpoints changed writable guest process state during
backboardd initialization. Developer Mode remained off.

The shared-cache slide for this boot is `0x14c84000`. All unslid addresses
below are specific to the iPhone18,3 24A437 cache; do not reuse them for
another build or assume the same slide on another boot.

## Enable the existing path

1. Stop backboardd with `/bin/launch-probe stop com.apple.backboardd`.
2. Attach LLDB to the guest QEMU GDB endpoint and set a hardware breakpoint
   at `0x184740138 + slide`. This is after the release-build path writes
   zero to the cached CADeviceUseVirtualMainDisplay flag.
3. Resume and run `/bin/launch-probe start com.apple.backboardd` over UART.
4. At the initializer breakpoint, write one byte, value 1, at
   `0x1e3fdb9d8 + slide`. Read it back, remove the breakpoint, detach.
5. Let startup complete and query with `/bin/display-probe`. An initial
   query aborted on its render-server wait; a later fresh process completed.

Verified result: six displays, including `CAVirtualMainDisplay`, ID 1,
external=0, supported=1, with mode `416 x 496 internal_panel (fixed)`.
However `CADISPLAY_MAIN_PRESENT=0`: the name alone does not establish it
as the actual main display. The five original Wireless displays remain.

## Main-display name matching

The CADisplay inventory/selection routine at `0x1844ea824` tests a display's
`name` at `0x1844eac00`. It recognizes CFStrings `LCD` and `Internal` before
storing the retained object in the main-display slot at `0x1e7025308`.
The strings are at `0x18480a422` and `0x18480a419`, respectively.

The built-in virtual path creates a three-entry options dictionary at
`0x18475d46c`: default dimensions 416 by 496 and the name
`CAVirtualMainDisplay`. A second experiment stops/restarts backboardd,
applies the flag change above, then stops at `0x18475d4b8 + slide`.
Here x8 holds the name CFString, just before it is stored in the dictionary
values array. Change x8 from `0x1e9014920 + slide` to the existing LCD
CFString `0x1e9013900 + slide`. Remove breakpoints and detach.

This supplies the name through the existing constructor rather than
assigning a fabricated main-display pointer or skipping a SpringBoard
assertion. In v42 the runtime addresses were `0x1993c4138` (flag breakpoint),
`0x1f8c5f9d8` (flag), `0x1993e14b8` (name breakpoint), and `0x1fdc97900`
(LCD CFString). The register change and flag readback both succeeded.

StopJob and RemoveJob returned errno 0 during these experiments. SpringBoard
was temporarily removed to stop repeated Mach-demand launches while testing.

## Verified LCD result and next failure

A fresh query after startup completed with:

```text
CADISPLAY_COUNT=6
CADISPLAY_ENTRY=0 ID=1 NAME=LCD EXTERNAL=0 SUPPORTED=1
CADISPLAY_ENTRY=0 CURRENT_MODE=<CADisplayMode 416 x 496 internal_panel (fixed)>
CADISPLAY_MAIN_PRESENT=1
CADISPLAY_MAIN_NAME=LCD
```

The other five displays remain zero-sized Wireless outputs. This proves
client-visible main-display selection; it does not prove pixels presented,
GPU operation, touch input, or an interactive SpringBoard.

Re-submitting the existing SpringBoard probe job returned errno 0. A hardware
breakpoint at kernel exception triage caught exception 6, codes
`[1, 0x1d9bbf5f4]`. The user trap message at `0x7c08c5fe91` was:

```text
invalid pid for <inert:[anon<SpringBoard>:-1]*>
```

The stack now runs through
`-[FBWorkspaceEventDispatcher registerSourceWithProcessHandle:].cold.3`,
workspace dispatcher initialization, FBProcessManager, FBSceneManager,
FBSystemShellInitialize and SBSystemAppMain. See the accompanying
[symbolication](springboard-after-lcd-v42-symbols.json). The main-display
assertion is no longer the observed blocker. Missing RunningBoard service
and the diagnostic user-domain launch are candidates for investigation;
this evidence does not yet distinguish their contributions.

All hardware breakpoints were deleted and LLDB detached. Both display
changes are ephemeral process state: restarting backboardd or the VM loses
them. No display pixels have been captured from this new virtual LCD yet.
