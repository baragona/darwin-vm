# V19 kernel breakpoint observations

Exact iOS 27.0 24A437 diagnostic bootkc; kernel slide 0x20000000.
Configuration matches v18; QEMU added `-S -gdb tcp:127.0.0.1:63419`.
These are transcribed debugger observations, not an automatic trace file.

Hardware breakpoint hit order (live addresses):

1. 0xfffffe00295ca83c: AppleKeyStoreUserClient startup entry.
   x0=0xf1fffe1b3416d300, x1=0xf2fffe210003dc00,
   x30=0xfffffe002b2e565c.
2. 0xfffffe00295ca898: base startup returned x0=1.
3. 0xfffffe00295ca8c0: passed the intervening bookkeeping calls.
4. 0xfffffe00295caaa4: passed the entitlement-check sequence.
5. 0xfffffe00295cab10 was not reached during the observation interval;
   neither were later checkpoints 0xfffffe00295cab24 / 0xfffffe00295cab48.
   QMP stop subsequently paused the CPU in its idle path.

Object reads while paused (top-byte tags removed for memory reads):

- Provider + 0xa8 at 0xfffffe210003dca8 = 0xfcfffe1b341ae070.
- Client + 0x110 at 0xfffffe1b3416d410 = 0xfefffe2100109a30.
- Work-loop vtable at 0xfffffe1b341ae070 = 0xfffffe0027ebad88.
- Vtable slot +0xa0 at 0xfffffe0027ebae28 = 0xfffffe002b32daf4.
- Command-gate object at 0xfffffe2100109a30 has vtable
  0xfffffe0027ebb078 and owner 0xf1fffe1b3416d300 at +0x18.

Disassembly between checkpoints shows command-gate creation stored at client
+0x110, provider getWorkLoop (a direct load from +0xa8), then work-loop virtual
call +0xa0 at 0xfffffe00295cab0c. The live slot resolves to the implementation
matching IOWorkLoop::addEventSource: it accesses controlG at +0x20 and invokes
its runCommand method with operation zero and the new event source.

Inference: startup is waiting inside addEventSource or its callees. The exact
lock owner and sleeping kernel-thread stack have not yet been recovered; this
does not establish that the missing SEP alone caused the wait.

All seven hardware breakpoints were removed, Developer Mode state was changed
from the verified TXM address to 1 and read back, and LLDB detached. QMP then
reported running=true. No code instructions were patched in this experiment.
