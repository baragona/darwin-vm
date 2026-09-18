# V25: writable /var, sandbox denial in path creation

Shared-cache slide 0xd914000. Transcribed debugger observations:

- `-[MCMError initWithErrorType:]` at live 0x21485db04 receives x2=91.
  LR=0x214881284 belongs to `____containermanagerd_start_xpc_block_invoke`.
- Breakpoint immediately after the call at unslid 0x206f6cf24 (live
  0x214880f28) reads x0=0. Receiver x25=0x7ba10100f0 is MCMLibraryRepair.
  The objc stub at 0x2080d41e0 selects `createPathsIfNecessaryWithError:`.
- Out-error at [sp+0x60] is 0x7ba1010210. Its code is 1 and domain object
  is 0x1f67e6768. The exact-cache constant string resolves to
  NSPOSIXErrorDomain, hence EPERM.
- Serial log independently reports:
  Sandbox: containermanagerd_system(...) deny(1) file-write-create
  /mnt1/root/Library/MobileContainerManager

This is not a write-capacity failure: the shell's /var write/readback and
root/mobile numeric ownership are verified separately. The symlink resolves
to a mount-point path outside this service's sandbox allowance. Adding writable
storage via a symlink is therefore insufficient for the full service stack.

All breakpoints were removed; no error return was overridden. Root APFS is
still read-only. Tmpfs is volatile and lacks a claim of complete APFS feature
compatibility. No graphical startup success is established.
