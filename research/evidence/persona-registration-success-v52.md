# V52: persona attributes and SpringBoard application registration

This follows the inconclusive crash documented in persona-attributes-v52.md.
The automatically restarted UserManager PID 352 had executable base
0x102944000; task inspection confirmed shared-cache slide 0xfb70000. The
guest developer byte was temporarily enabled for thread-probe and restored
and read back as zero before the installer experiment.

At the list helper call 0x10294906c, x0 and owned x20 were nil. Traversing
the fresh daemon state gave:

- global 0x102a2b398 -> manager 0x7bc6c14040
- manager+0x20 -> state 0x10315d430
- state+8 -> dictionary 0x10315d950
- dictionary+8 -> storage 0x10315df00
- mobile key 0x7bc7008500, text at +0x20 verified as
  CE7B39AA-4774-5BA4-BD1C-A29AB22DB754

Changing only borrowed x0 at the list call let execution reach the per-persona
helper call at 0x10294770c. There x0=0x7bc6c202a0, x1=nil, owned x20=nil.
Changing borrowed x1 to the freshly validated mobile key yielded a non-null
dictionary x0=0x10315f2f0 at 0x102947710 and error *(int*)(fp-0x34)=0.
The same substitution was applied to remaining attribute requests.
InstalledContentLibrary at 0x1bac003d8 then observed x0=4 (attribute count),
x25=0x7cc8c18270. Installer PID 357 remained running, LAST_EXIT=0.

The two temporary LLDB hardware-breakpoint commands are:

```text
breakpoint set -H -a 0x10294906c
breakpoint command add -o 'register write x0 0x7bc7008500' 1
breakpoint set -H -a 0x10294770c
breakpoint command add -o 'register write x1 0x7bc7008500' 2
breakpoint modify --auto-continue true 1 2
```

These addresses are valid only for this live daemon. Do not reuse them after
a restart. This selects one synthetic mobile-user namespace for these calls;
it does not allocate kernel personas or establish faithful session identity.
It fixes the observed lookup in this experiment, not general provisioning.

URL registration still returned false. Installer stderr identified missing
/var/installd/Library/MobileInstallation. That directory was created before
the rebuild. An attempted /usr/sbin/chown failed because the program was absent;
/bin/chown -R 33:33 /var/installd succeeded afterward, before the fresh query.

The system-only rebuild used the existing probe, UID pointer 501, with a
one-shot diagnostic entitlement-gate change at 0x196a56528 (unslid
0x186ee6528): x26=0 was changed to 1; that breakpoint was removed immediately.
This permits the operation; it does not fabricate the API result. The rebuild
returned true. Crucially, a separate process subsequently returned:

```text
LS_PROXY=<LSApplicationProxy: 0x1050d03a0> com.apple.springboard file:///System/Library/CoreServices/SpringBoard.app/ <com.apple.springboard <installed >:0>
LS_IS_INSTALLED=1
LS_BUNDLE_URL=file:///System/Library/CoreServices/SpringBoard.app/
```

Evidence: [fresh daemon](persona-fresh-threads-v52.txt),
[installer start](persona-fresh-start-v52.txt),
[installer stays alive](persona-installer-alive-v52.txt),
[installer output](persona-installer-output-v52.txt),
[URL attempt](persona-register-springboard-v52.txt),
[rebuild](persona-rebuild-v52.txt),
[fresh verification](persona-after-rebuild-v52.txt).

The next experiment restores the previously verified virtual LCD and launches
SpringBoard. Application registration alone does not prove an interactive GUI.

## SpringBoard advances to lock-screen credentials

Backboard was restarted, applying the existing virtual-main flag and LCD
name substitutions at 0x1942b0138 and 0x1942cd4b8. Each breakpoint fired once
and was then deleted. The first display probe timed out observing startup
and later aborted; a fresh probe completed with six displays, main LCD,
416x496. See [display verification and launches](persona-display-runningboard-v52.txt).

RunningBoard PID 378 and SpringBoard PID 380 launched successfully. A later
[job query](persona-springboard-result-v52.txt) showed SpringBoard still
running with LAST_EXIT=0 and empty stderr. This is not yet a usable UI.
The [thread sample](persona-springboard-threads-v52.txt) locates its main
thread in IOKit transport while initializing lock-screen credentials:

```text
io_connect_method / IOConnectCallMethod / IOConnectCallStructMethod
performCommand / ioKitTransport
LibCall_ACMContextCreate / ACMContextCreateWithFlags
-[SBFCredentialSet initWithSerializedCredentialSet:]
-[SBLockScreenBiometricAuthenticationCoordinator initWithBiometricResource:walletPreArmController:]
-[SBLockScreenCoordinator initWithWindowSceneManager:biometricResource:walletPreArmController:activeDisplayPolicy:]
-[SpringBoard applicationDidFinishLaunching:] + 6232
```

[Symbol lookup results](persona-springboard-wait-symbols-v52.json) and repeated
serial messages `ACMTRM: waitForSEPEndpoint: timed out waiting for
AppleSEPManager (timeoutMs=5000)` identify the next investigation. The frame
walker is heuristic; these are address-to-symbol mappings, not a validated
full unwind. No Secure Enclave response or successful credential context is
claimed.

Current live state: V52 remains running, LLDB session attached with only the
two auto-continuing persona argument substitutions. They depend on UserManager
PID 352 and must be discarded on its restart. Display initialization and
rebuild-authorization breakpoints are removed. The guest developer byte was
again restored and read back as zero after the SpringBoard thread sample.
No host security settings changed. Registration and directory changes are in
volatile /var; boot reproduction still requires the documented steps.
