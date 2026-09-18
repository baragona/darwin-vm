# Experimental SPTM invalidation substitution, v52 / 24A437

V51 reproduced the terminal panic at PC 0xfffffe00071024e0, ESR 0x02000000,
ending in nested-panic-limit/reset-or-spin. The debugger confirmed the guest
in a panic spin loop; all breakpoints were deleted and QMP quit stopped it.
The fault word 0x0020134d also exists at file offset 0xfe4e0 in the original
SPTM. This instruction was not introduced by our persona/debugger changes.

Static control flow at unslid 0xfffffff027102030 loads a 16-bit field into
x13, splits its low-byte index and high-byte generation, compares the latter
with a byte table, and reaches 0xfffffff0271024e0 on mismatch. This occurs
near TTBR0_EL1/TCR_EL1 updates. Other custom 0x002013xx operations have
page-address operands and surround DSBs. These observations motivate an
**invalidation hypothesis**, not a verified instruction specification.

The hash-guarded patcher replaces only 0x0020134d with 0xd508831f,
`tlbi vmalle1is` (full EL1 translation-cache invalidation). This is a guest
firmware experiment, not an upstream-ready QEMU implementation. It preserves
the source file and requires exclusive creation of the output.

- Original SHA-256: d4b9a9db5d383734585a075da7afdd38a72ec7f5d43dbbd54708f07841fd5734
- Patched SHA-256: 8445960f9821c01d41710ed66abcf189d6a3612288bad1651ae43d8323848234
- SPTM UUID: 325C14BC-3EFA-33A3-9E5C-7A7049E321CD; source version 820.0.22
- File size unchanged; bytes before and after the one instruction identical.

V52 uses the v51 service-root and populated seed, with only the SPTM override
changed. It boots to UI_PROBE_END; cache slide 0xfb70000. A hardware breakpoint
at the patched runtime PC fired during the 300-process stress command, near
PID 82. Registers: x13=0x102, x24=0, x25=1, matching the prior panic's operand
and the static generation mismatch. The breakpoint was removed and debugger
detached. An attempted immediate PC read after asynchronous single-step was
rejected because the target was running; no successful single-step readback
is claimed. Continued serial execution supplies the outcome instead.

The test command is recorded in sptm-process-stress-v52.txt. This test addresses
the repeated process-churn panic, not GUI, translation correctness under all
workloads, the semantics of related custom opcodes, or kernel persona setup.

Result: the serial command completed with PROCESS_STRESS_COMPLETED=300 and
its completion marker, reaching PID 336 without the prior SPTM panic. The
substitution was actually reached (hardware breakpoint evidence above), so
this is stronger than merely booting with a patched but unexecuted instruction.
