# Persona attribute failure, v52

V52 uses the experimental SPTM invalidation patch documented separately.
The populated synthetic persona manifest is unchanged. These observations
are debugger diagnostics, not a working persona provisioning fix.

## Verified installer request

The explicit installer start created PID 348 (see
[serial transcript](installer-explicit-start-v52.txt)). UserManager's request
argument x2 was 0x15c, confirming the caller PID. Its executable base was
0x104044000; the shared-cache slide was 0xfb70000.

At UserManager 0x104049064, the caller-to-user resolver returned nil.
A one-shot substitution with the existing mobile-user NSString
0x7d8d008500 made enumeration return four entries (x0=4 at 0x104049140).
At the actual client's raw enumeration return, 0x19e3d49f0, x0 was
0x7524c18180 and the error slot at sp+0x68 was zero.

The subsequent per-persona attribute return at 0x19e3d4ae4 had x0=nil.
The error slot at sp+0x18 pointed to NSError 0x7524c18360, with
NSPOSIXErrorDomain, code 2 (ENOENT), and no userInfo. Thus successful raw
enumeration does not establish success of the public attribute-list API.
Static inspection of UserManagement shows its error path returns an empty
array while also setting NSError. InstalledContentLibrary then sees a zero
count and reports an empty list. Its provider was identified as UMUserManager.

## Inconclusive follow-up and cleanup

A later attempt set borrowed helper arguments at UserManager 0x10404906c
(x0, list lookup) and 0x10404770c (x1, attribute lookup) to the existing mobile
UUID object. Correct LLDB setup uses one `breakpoint command add -o` command
per breakpoint plus `breakpoint modify --auto-continue true`; repeated `-o`
options retained only the last command in an earlier, ineffective attempt.

The corrected attempt launched installer PID 351. The list breakpoint fired,
but UserManager PID 339 then exited with SIGSEGV before the attribute
breakpoint fired. The installer again observed an empty list. This does not
validate or disprove the attribute substitution: the crash cause was not
captured, and the reused object came from an already-mutated daemon. A fresh
daemon and fresh object validation are necessary before repeating it.
See [launch transcript](persona-two-lookups-start-v52.txt) and the accompanying
serial-log excerpt below.

All debugger breakpoints were deleted and LLDB detached. The temporary
guest developer byte had already been restored to zero. No host security
settings or persistent persona identity patch changed. Installer removal and
service state are recorded in [cleanup](persona-cleanup-v52.txt).

Next useful experiment: trace caller identity and the per-persona attribute
lookup in a fresh daemon, checking the requested persona key against the seed.
Do not rerun stock `usermanagerd --init` without addressing its previously
verified RAM-disk/APFS boot-device failure. No interactive SpringBoard yet.

## Serial-log excerpt

```text
e3076fb351771
bash: e3076fb351771: command not found
com.apple.xpc.launchd|1970-01-01 00:05:54.908123 (user/501/com.apple.mobile.installd [351]) <Notice>: xpcproxy spawned with pid 351
com.apple.xpc.launchd|1970-01-01 00:05:54.908269 (user/501/com.apple.mobile.installd [351]) <Notice>: internal event: SPAWNED, code = 0
com.apple.xpc.launchd|1970-01-01 00:05:54.908342 (user/501/com.apple.mobile.installd [351]) <Notice>: service state: xpcproxy
com.apple.xpc.launchd|1970-01-01 00:05:54.909176 (user/501/com.apple.mobile.installd [351]) <Notice>: internal event: SOURCE_ATTACH, code = 0
System Policy: xpcproxy(351) allow process-exec* /usr/libexec/installd
com.apple.xpc.launchd|1970-01-01 00:05:55.034273 (user/501/com.apple.mobile.installd [351]) <Notice>: service state: running
com.apple.xpc.launchd|1970-01-01 00:05:55.034452 (user/501/com.apple.mobile.installd [351]) <Notice>: internal event: INIT, code = 0
com.apple.xpc.launchd|1970-01-01 00:05:55.034559 (user/501/com.apple.mobile.installd [351]) <Notice>: Successfully spawned installd[351] because non-ipc demand
com.apple.xpc.launchd|1970-01-01 00:05:55.090503 (pid/351 [installd]) <Notice>: uncorking exec source upfront
com.apple.xpc.launchd|1970-01-01 00:05:55.090663 (pid/351 [installd]) <Notice>: created
com.apple.xpc.launchd|1970-01-01 00:05:55.176571 (user/501) <Warning>: failed lookup: name = com.apple.logd, flags = 0x9, requestor = installd[351], error = 3: No such process
Sandbox: installd(351) deny(1) file-read-data /var
com.apple.xpc.launchd|1970-01-01 00:05:55.291897 (user/501/com.apple.mobile.usermanagerd [339]) <Notice>: exited due to SIGSEGV | sent by exc handler[339], ran for 116752ms
com.apple.xpc.launchd|1970-01-01 00:05:55.292012 (user/501/com.apple.mobile.usermanagerd [339]) <Notice>: service has crashed 1 times in a row (last was dirty)
com.apple.xpc.launchd|1970-01-01 00:05:55.769872 (user/501/com.apple.mobile.installd [351]) <Notice>: exited with exit reason (namespace: 18 code: 0x2) - OS_REASON_LIBSYSTEM | Failed to get installd daemon container: Error Domain=MIInstallerErrorDomain Code=4 UserInfo={NSUnderlyingError=0x7c7ec18360 {Error Domain=NSPOSIXErrorDomain Code=2 "No such file or directory"}, FunctionName=<private>, SourceFileLine=207, NSLocalizedDescription=<private>}, ran for 859ms
com.apple.xpc.launchd|1970-01-01 00:05:55.769992 (user/501/com.apple.mobile.installd [351]) <Notice>: service has crashed 2 times in a row (last was dirty)
com.apple.xpc.launchd|1970-01-01 00:05:55.770052 (user/501/com.apple.mobile.installd [351]) <Notice>: service state: exited
com.apple.xpc.launchd|1970-01-01 00:05:55.770112 (user/501/com.apple.mobile.installd [351]) <Notice>: internal event: EXITED, code = 0
com.apple.xpc.launchd|1970-01-01 00:05:55.770303 (user/501/com.apple.mobile.installd [351]) <Notice>: service state: not running
com.apple.xpc.launchd|1970-01-01 00:05:55.770928 (pid/351 [installd]) <Notice>: shutting down
com.apple.xpc.launchd|1970-01-01 00:05:55.770978 (pid/351 [installd]) <Notice>: cleaning up
com.apple.xpc.launchd|1970-01-01 00:05:55.771054 (system) <Notice>: removing child: pid/351
```
