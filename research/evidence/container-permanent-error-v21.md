# Container helper permanent error, v21

Shared-cache slide 0x4e8c000. These are transcribed live debugger observations.

1. Hardware breakpoint at `_exit` (unslid 0x187f23b6c) hit with x0=0,
   LR=0x20bdfa55c. Symbolicating LR-4 identifies
   `____containermanagerd_reply_with_error_block_invoke` at 0x206f6e534.
   The exact block loads w0=0 and calls exit; status 0 is not startup success.
2. A breakpoint at
   `____containermanagerd_listener_handler_for_permanent_error_block_invoke`
   (unslid 0x206f6e2c8) hit on the next managed request.
3. Block x0=0x7a79010060; captured error at block+0x28=0x7a79014030.
   Error ISA=0x020000027588e039 maps to MCMError class at unslid 0x270a02038.
   Error +0x10=0x66 (102). Disassembly of -[MCMError type] verifies +0x10.
4. `_container_get_error_description` at 0x207057a24 indexes a pointer table
   at 0x2683448d8. Live entry [102] at 0x26d1d0c08 is 0x20bf01d1d, containing:

       USER_HOME_DIRECTORY_MISSING

All hardware breakpoints were deleted before detach. V21 was subsequently
stopped for the home-directory experiment.

Separately, mount output confirmed `/dev/md0 on /` is read-only APFS.
Touching /private/var/tmp/write-test failed. mount_tmpfs on that existing
directory was denied by System Policy (file-mount), and the mount failed with
Operation not permitted. These storage constraints are verified independently;
they do not by themselves explain which home the container helper expected.
