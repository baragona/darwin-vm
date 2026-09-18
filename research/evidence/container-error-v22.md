# Container helper error after adding home directories

V22 retains the v21 firmware and service configuration. The only image change
was creating /private/var/root and /private/var/mobile, mode 0755, on the host
ownership-disabled mount. This does not establish correct root/mobile ownership
or writable storage. No tmpfs mount succeeded.

Shared-cache slide: 0x1be2c000. A hardware breakpoint at the same permanent-error
handler (live 0x222d9a2c8) captured block 0x7427014180. Its error pointer at +0x28
was 0x7426c18060. Error type at +0x10 was 0x95 (149), changed from v21's 102.

Offline exact-cache inspection:

    ipsw dyld dump DSC 0x268344d80 -a -c 1
    -> 0x207076177
    ipsw dyld dump DSC 0x207076177 -s 128
    -> INVALID_CONFIG_FILE

Table address is 0x2683448d8 + 149*8, independently derived from
_container_get_error_description's disassembly. An attempted live read while
the CPU was in the kernel idle address space failed; offline cache inspection
avoided relying on that unavailable user mapping.

Backboardd still aborted and CADisplay did not return a display count. All
breakpoints were removed and the debugger detached. The next specific lead is
the original ContainerManagerCommon.framework plist resources, which exist on
the full system image but were not staged with the shared-cache-only setup.
