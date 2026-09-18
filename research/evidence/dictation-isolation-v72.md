# Dictation notification isolation with Setup bypassed (V72)

This is a negative result. Suppressing UIKit's dictation-availability callback
alone did not improve the captured UI. No new dictation override remains.

## Object-scoped breakpoint trial

At -[UIDictationConnection dictationConnnectionDidChangeAvailability:],
unslid0x1858123c0, runtime0x18864e3c0, the observed receiver was
0x02000079936fed00 and argument0x02000079898524c0. The return address was
0x19bed47ec in the AssistantServices availability callback.

A conditional breakpoint for untagged receiver0x79936fed00 returned immediately
by setting PC to LR. It reported7,900 hits before removal. This generated
substantial debugger overhead. The UART command eventually completed, but its
capture occurred after the override was removed, so that capture is not a valid
measurement of behavior under the override. It still showed the same status bar.
The transcript and decoded capture use the prefix dictation-ui-quiet-v72.

## Instruction-patch trial

To remove breakpoint overhead, temporarily replace the callback's first
instruction in the live guest with RET:

- address:0x18864e3c0
- original:0xf9401808 (ldr x8,[x0,#0x30])
- temporary:0xd65f03c0 (ret)

The original word was read before writing; the temporary word was read back.
This second trial was not object-scoped: it suppressed this UIKit callback in
the guest mapping being patched. Shared-cache propagation to other guest
processes was not measured. No host security setting or disk firmware was edited.
The method has not entered its authenticated prologue at this point, so returning
through LR does not skip a required stack unwind.

While the patch remained installed, the guest completed wake, virtual swipe,
capture, compression, base64 export, and launch-job listing with the completion
marker. All14 matching HID callbacks arrived; SpringBoard remained PID186.
The decoded BGRA SHA256 was
 a03ac4b3e3f9f39a3c9054b9ffd833018077cb78362bfd3b70a7d8c63e3ddc43
with2,624 colored pixels: byte-for-byte the same status-bar-only image as the
previous home-swipe-capture-v72 result. No home screen or unlock was demonstrated.
See dictation-ret-capture-v72.txt/.json/.png.

After the completed test, the patched word was read as0xd65f03c0, restored to
0xf9401808, and read-verified. Guest execution resumed. All dictation breakpoints
from this experiment were removed or one-shot; the earlier Setup-required-reason
bypass remains active.

This result does not prove that dictation activity is harmless, nor test restoring
assistantd or draining every already-queued notification. It does show that this
single callback suppression is insufficient to recover visible app content in
the current run. Investigate the actual UI-lock/cover-sheet transition next
rather than retaining an ineffective callback patch.
