# Extension discovery service: resources restored, launch path rejected

V52 lacked the entire ExtensionFoundation.framework/XPCServices/
extensionkitservice.xpc bundle. The failing SpringBoard logged a service lookup
failure immediately before the previously captured Swift cast abort.
[Guest file check](extension-service-files-v52.txt).

V52 was stopped with QMP quit after deleting its debugger breakpoints. The
matching 24A437 XPC bundle was copied into the stopped service-root image.
An initial V53 boot showed that the parent framework had no Info.plist or
configuration resources either; V53 was stopped before attempting SpringBoard.
The remaining matching framework files were added. All eight files in the
resulting framework tree were checked byte-for-byte against the mounted
firmware. [Initial additions](extension-service-image-v53.json),
[complete framework manifest](extension-framework-image-v54.json).
No firmware binaries are included in these commits.

A separately signed /bin/ls-rebuild-probe was copied from the already installed
corrected registration probe and signed with only
com.apple.lsapplicationworkspace.rebuildappdatabases. Its hash was added to the
version-1 trust cache without removing entries (4399 -> 4400). The extension
service hash was already present. The original unentitled probe is unchanged.
The first codesign metadata extraction used insufficient verbosity and failed
before trust-cache mutation; `codesign -dvvvv` provided the verified CDHash.

## V54 reproduction

The new boot uses the existing SPTM invalidation experiment, no-SEP device tree,
AKS endpoint-order bootkc, and 16 GiB memory. Cache slide 0x13954000.
UserManager PID 38 has executable base 0x104c34000. The mobile-user NSString
0x794700c910 was re-read from this daemon's dictionary and its UUID verified
before reuse. Guest developer override was enabled only for task inspection,
then restored and read back as zero.

[Debugger command snapshot](extension-startup-v54.lldb) records the temporary
persona, virtual LCD, credential-failure and backlight-filter setup. This is
historical evidence, not a reusable address-independent script. In particular,
the backlight filter condition reads the current CADisplay's ID and applies
the diagnostic only to ID 1. The credential breakpoint's two-command list was
verified before continuing.

The [entitled rebuild](extension-rebuild-v54.txt) returned true without a
rebuild-authorization debugger override. A fresh unentitled query reported
SpringBoard isInstalled=1 and the correct bundle URL. The transcript observer
timed out because a concurrent kernel log interleaved with its completion
marker, not because the rebuild or following commands failed.

[Display verification and application launch](extension-springboard-start-v54.txt)
again show six displays and a 416x496 main LCD. Installer PID 48 remains up;
RunningBoard PID 58 and SpringBoard PID 60 launched. Both credential and
backlight-filter substitutions fired. However launchd now explicitly rejects
the discovered framework service:

> Path not allowed in target domain: type = pid ... error = 147: The specified service did not ship in the requestor's bundle

For SpringBoard, the reported origin is /System/Library/CoreServices/SpringBoard.app.
[Exact path-rejection lines](extension-path-rejection-v54.txt).
The service's original Info.plist says XPCService.ServiceType=Application and
advertises Discovery, Launch, Observer, and TCCProxy BSServiceDomains.
The same path rejection also occurs for backboardd and installd.

SpringBoard still aborts with the _NSXPCDistantObject ->
ExtensionFoundation._EXDiscoveryServiceProtocol cast failure, including after
multiple launches. [Stderr and job result](extension-result-v54.txt).
Restoring files therefore does not resolve this issue; the next task is the
guest launch-domain/path eligibility check. Error 147 is observed directly;
its underlying policy cause and relationship to the Swift cast are not yet
proven. Do not bypass the cast blindly.

## Current state

V54 is running at /tmp/a19-ui-v54-qmp.sock, UART
/tmp/a19-ui-v54-serial.sock, GDB 127.0.0.1:63454. LLDB remains attached with only
the two persona argument substitutions for UserManager PID 38. Display-init,
credential, and backlight-filter breakpoints were removed after testing.
SpringBoard removal returned errno 36 during launch teardown; verify actual
job removal before resubmitting. Guest developer override remains zero.
No host security settings changed. Interactive SpringBoard is not achieved.
