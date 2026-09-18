# Extension service rejection: root ownership check

## V54 finding

Launchd error 147 ("did not ship in the requestor's bundle") was caused by
ownership of the newly copied framework service in this experiment. It was
not evidence that an additional launch domain was required.

For build 24A437, launchd's framework-path helper returned true for the
ExtensionFoundation service, just as it did for two working Apple framework
services. The subsequent stat-buffer check required UID 0 and rejected group
or other write permissions. The extension bundle was mode 0755 but UID/GID
99:99; the working comparison bundles were 0:0.

At launchd executable base 0x102474000, the check after the path helper was
0x10249dd0c (unslid 0x100029d0c). The stat structure was at
0x010000769a838060; UID at offset 0x10 was 0x63. A single debugger write of
zero to that UID slot let extensionkitservice launch as PID 75 in SpringBoard
PID 74's process domain. This changed the temporary stat result, not disk
ownership or launchd code. Subsequent launches again encountered UID 99.

SpringBoard advanced past the discovery-protocol cast failure and ran for
about 81 seconds, then aborted with NSInternalInconsistencyException:
"Luminance Map Image must be a CGImage based image". Its stderr also reported
missing Helvetica.ttc, SFUI.ttf and LastResort. Interactive graphics was not
achieved. [Guest output](extension-owner-cleanup-v54.txt).

## Image ownership lesson

Files copied into an APFS image mounted on the host with `-owners off` can
appear as the host user in host stat output but appear as UID/GID 99 in this
guest. Verify ownership inside the guest, especially for framework XPC
bundles: launchd's error text did not mention the actual ownership failure.

The guest root was read-only; mount -uw / failed. A live tmpfs mount over the
bundle was denied by guest System Policy. Host chown failed, and noninteractive
sudo required an unavailable password. No host policy settings were changed.
[Ownership and remount](extension-owner-guest-v54.txt),
[tmpfs attempt](extension-owner-overlay-v54.txt).

## V55 experiment

With V54 stopped, the original eight framework files were moved to
/var-seed/System/Library/Frameworks/ExtensionFoundation.framework. The System
framework path now points to /private/var/System/Library/Frameworks/ExtensionFoundation.framework.
The existing writable-var boot script copies the seed and recursively sets
root ownership before launching services. This is an experimental workaround
for the disposable restore guest, not a recommendation for a normal OS image.

The missing SpringBoard.framework resource tree and the three named Core
fonts were also copied from matching firmware. All 156 added resource files
were byte-compared with the source. No Apple firmware is committed.
[Image manifest](extension-owner-resources-v55.json).

V55 verified root:wheel ownership of the target service bundle, but extension
service lookup failed with error 3 and the Swift cast abort returned. There
was no successful extension launch. Thus the full-framework symlink is not a
working solution. Its precise discovery failure has not yet been traced.
[Ownership check](extension-owner-check-v55.txt), [result](extension-result-v55.txt).

The entitled rebuild again returned true, and the display probe reported six
displays with a 416x496 main LCD. Cache slide 0x1144c000; UserManager PID 39,
base 0x102d78000; verified mobile key 0x748500c550.
[Startup debugger snapshot](extension-startup-v55.lldb).
Guest developer inspection byte was restored and read back as zero. All
breakpoints were deleted, LLDB detached, and V55 stopped with QMP quit.

The V56 image restores the framework as an actual directory and narrows the
symlink to its XPCServices subdirectory. The SpringBoardFoundation,
PosterUIFoundation and CoreMaterial framework resource trees were also
entirely missing; these contain matching luminance-map images. Their 104
files (1,178,528 bytes) were copied and byte-compared with firmware. This is a
resource hypothesis, not yet a proven fix for the image assertion.
[Manifest](extension-layout-resources-v56.json). V56 runtime test pending.
Historical debugger addresses must be re-derived after every boot.

## V56 linked-service check

At libxpc `_xpc_bundle_taint_with_reason` (unslid 0x1805dc120,
runtime 0x18d944120; cache slide 0xd368000), x1 is the string
"linked xpc services", x0's bundle path is the System ExtensionFoundation
framework, and x24/x25 contain the resolved service path under /private/var.
LR is 0x18d94499c in `_xpc_bundle_resolve_services`. The narrowed symlink
is therefore also recognized as a linked service. Launchd then rejects the
resolved /private/var path with error 147. Root ownership alone is insufficient
when moving the service outside its original trusted framework path.

An attempted `register write pc $lr` was rejected by LLDB (requires a numeric
value); no taint-call override occurred. The breakpoint was deleted and the
guest continued normally. SpringBoard still reports the discovery cast abort.

V56: UserManager PID 38, base 0x1021a8000, verified mobile NSString
0x7847010410. Developer byte restored/read back as zero. Entitled rebuild
returned true; virtual LCD is again the main 416x496 display.

### One-shot path diagnostic in V56

Launchd base 0x100974000 was derived from _xpc_bundle_get_path LR
0x10098e21c and verified by Mach-O magic 0xfeedfacf. At 0x10099dd0c,
AppleDeviceQuerySupport and CoreAccessories both returned framework helper
value 1 with UID/GID 0:0. The relocated extension service returned 0 with
UID/GID 0:0 and mode 0755 (stat buffer 0x0600007a3933f760).
Thus ownership was corrected, but the new resolved path is ineligible.

A single `register write w0 1` at that check for the verified extension path
allowed launch; the breakpoint was immediately deleted. This is a diagnostic
policy override, not a persistent filesystem fix. No taint override was used.
Extension service PID 65 launched in SpringBoard PID 64's process domain at
guest time 00:04:03.698885. [Observation](extension-path-override-v56.txt).

## Next exception and current state

The resource additions did not remove the luminance assertion. At the next
objc_exception_throw (runtime 0x18d778328), the exception reason was read as
"Luminance Map Image must be a CGImage based image". The symbolicated stack
identifies `+[UIColorEffect colorEffectLuminanceMap:blendingAmount:]`,
`-[CSProminentButtonControl _backgroundEffectsWithBrightness:]`,
`-[CSProminentButtonControl initWithFrame:luminanceMap:]`, and
`-[CSQuickActionsView _createButtonForAction:]`. This localizes the failure
to lock-screen quick-action controls in CoverSheet/CoverSheetKit.
[Symbolication](extension-next-exception-symbols-v56.json),
[final stderr](extension-final-stderr-v56.txt). The QMP framebuffer capture
was entirely black; no usable graphics or input was demonstrated.

An attempted UART status command timed out while stopped on the exception;
that capture is not evidence of a hung shell. After continuing, a separate
stderr capture completed. SpringBoard removal returned errno 36. All remaining
breakpoints were removed, LLDB detached, and V56 was stopped with QMP quit.

The stopped image now restores the original ExtensionFoundation framework
and XPCServices as directories, without either experimental symlink. All
eight framework files match the source. This restores the known UID-99
ownership issue; it does not solve it. The unused seed copy remains.

Both CoverSheet and CoverSheetKit resource directories were absent. Their
19 files (2,534,105 bytes), including CoverSheet Assets.car, were copied from
matching firmware and byte-verified. [Manifest](coversheet-resources-v57.json).
This is the next targeted resource experiment, not a verified fix: V57 has
not booted. The image is detached, no VM is running, and no debugger is attached.

Next: preserve original framework paths while correcting ownership (or use
the documented, explicit one-shot ownership diagnostic), then validate the
CoverSheet resource addition. Do not treat symlink relocation as a fix or
blindly suppress the UIImage assertion. No host security settings changed.
