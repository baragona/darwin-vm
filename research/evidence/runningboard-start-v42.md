# RunningBoard startup and subsequent failures (v42)

The staged `/launchjobs/com.apple.runningboardd.plist` was submitted with the
legacy launch API. It preserves the firmware Program/MachServices and has
KeepAlive disabled. Submission returned errno 0, launchd reported PID 81,
and lookup of `com.apple.runningboard` returned result 0, port 6659.

SpringBoard was restarted as PID 84. It exited with SIGABRT after 5359 ms,
rather than the previous SIGTRAP from the inert process-handle assertion.
This is not enough to establish which initialization stage it reached.
The debugger was filtering exceptions 1 and 6, so it missed this abort.

Immediately after launchd scheduled another Mach-demand SpringBoard launch,
the guest panicked in SPTM. The first panic reports PC
`0xfffffe00071024e0`, opcode `0x0020134d`, ESR `0x02000000` (unrecognized
instruction). Kernel panic reporting then produced nested data aborts and
ended in its explicit reset-or-spin loop. LLDB confirmed the spin at
`0xfffffe002ad57a6c`; QMP running status by itself was not guest liveness.

The user frames below the kernel/SPTM stack symbolize to `posix_spawn`,
`posix_spawnp`, a main-executable frame at `0x1041f61f4`, and dispatch worker
frames. The spawning process identity has not been independently established.
See [first panic](runningboard-sptm-panic-v42.txt) and
[user-frame symbols](runningboard-panic-userspace-v42-symbols.json).

The opcode exists unchanged in the input SPTM at Mach-O VA
`0xfffffff0271024e0`, file offset `0xfe4e0`. Static code reaches it when a
cached high byte differs from the high byte of a 16-bit value loaded into
x13; adjacent code switches TTBR0_EL1/TCR_EL1 with barriers. This suggests
address-space maintenance, but the exact instruction semantics are not
verified. The emulator's apple-gxf decoder recognizes GENTER/GEXIT, not this
opcode. Do not replace it with a no-op based solely on this observation.

The v42 debugger was detached with breakpoints removed, and the terminally
panicked VM explicitly quit. A fresh v43 was booted from the same image to
catch the earlier SpringBoard abort. The temporary virtual LCD setup must
be reapplied; no guest-image or emulator changes were made in this test.
