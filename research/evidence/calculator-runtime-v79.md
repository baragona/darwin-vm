# Calculator launch investigation in V79

LaunchServices independently reported com.apple.calculator installed at
/Applications/Calculator.app. Opening it returned1, and launchd spawned
Calculator81. A fresh832x1808 framebuffer exported and strictly decoded all
6,017,024 bytes with every pixel opaque; SHA256
ab13b2cc62cae5386393b21d07002f4350df532d9abbf3e1eaea24be1db961c1.
Visual inspection shows a black screen except the diagnostic banner and a
small bottom artifact. There is no usable Calculator UI in this capture.
Calculator81 then exited with SIGTRAP after88.9 guest seconds.

A repeat spawned Calculator87, which also exited with SIGTRAP after56.7 guest
seconds. Read-only breakpoints on three Swift assertionFailure entry points
and swift_stdlib_reportFatalError did not fire. Adding os_crash, dispatch
queue assertion failure and abort_with_payload observers likewise did not
catch Calculator89's trap after54.4 guest seconds. These observations do not
establish the cause or rule out other runtime/framework trap paths.

A further launch is stopped at UIApplicationMain. Walking its frame pointers
found the app main return atbase+0x1ffc. Its Mach-O header contains UUID
19A87853-DD17-33B3-99DB-5C8AB6F0EC48, matching the local source Calculator.
Runtime base is0x104860000. The experiment arms hardware-style guest-debugger
breakpoints on the561 BRK#1 instructions found in that exact app binary;
it neither alters the app nor skips any invariant. PAC-related trap sites
are not included in this first direct-trap experiment.

Calculator arithmetic remains unverified. Successful process launch is not
completion of the Calculator part of the interactive-demo goal.

The direct-trap attempt remained alive long enough to sample Calculator91.
Its main thread was in os_trace_read_file_at -> os_trace_read_plist_at ->
os_log_preferences_load_base_sysprefs_file -> os_log_preferences_load ->
os_log_preferences_refresh -> os_log_create, called by Swift Logger creation
in Calculator. This is a sampled startup location, not a proven crash cause.
The experiment was narrowed to the12 direct traps below app offset0x10000
(after deleting549 later sites) to reduce debugger overhead around this
initialization path. No trap has yet been intercepted. Kernel task inspection
was switched back to0 from a kernel-context one-shot breakpoint after the
thread sample; a user-context write had failed and was not counted as restored.

V78 has now been closed after V79 demonstrated notebook restoration, leaving
only V79 running among the Darwin guests.
