# Foreground launch fails on missing lifecycle-policy resources

A deeper read-only decode of the original launch completion in V74 gives:

- FBSOpenApplicationServiceErrorDomain 3: ProcessExited.
- FBProcessExit 64: launch-failed.
- RBSRequestErrorDomain 5: Unable to execute launch request.
- RBSAssertionErrorDomain 2: Could not find plist for domain attribute.

`settings-open-assertion-v74.json` preserves that chain. The final single-entry
dictionary layout was checked against this firmware's CoreFoundation
getObjects:andKeys:count: implementation; its key/value offsets were read from
runtime ivar metadata. No error value or launch result was substituted.
The UART request completed with OPEN_EXIT=1 and its terminal marker.

The guest has no /System/Library/LifecyclePolicy directory. The matching system
firmware contains 147 files totaling 145,587 bytes there, including domains.plist
and com.apple.frontboard, com.apple.uikit and com.apple.coreos domain attributes.
This is a shared lifecycle prerequisite, not yet evidence of a Settings-specific
startup problem. The exact requested missing domain was not captured.

A live mkdir failed because the guest root is read-only. A guest-only
`/sbin/mount -uw /` returned 77 (Operation not permitted), and mkdir still failed.
No policy was installed in V74. The launch observers 15, 16 and 18 were removed;
the guest resumed with its existing UI bootstrap breakpoints.

Instead, service-root-touch-v77.dmg was cloned from the V74 image and populated
with the original LifecyclePolicy directory and the new minimal Touch Probe app.
An initial sandboxed copy could create empty files but not write policy contents;
the copy was repeated with the required filesystem access. Every installed file
was then compared to its source by SHA-256: 150 files, 217,704 bytes. The image
was cleanly unmounted. touch-v77.tc preserves 4,414 entries and adds the app's
signed hash, for 4,415. See touch-image-v77.json.

V77 is a separate test guest; V74 retains the working larger home screen.
Policy restoration has not yet been proven to resolve foreground launching.

V77 subsequently booted to its Bash shell and completed UI_PROBE_END. Its cache
slide is 0x2920000 (cache base 0x182920000), so V74 debugger addresses must not
be reused. The standard thread probes were denied while the guest inspection
flag was off; this does not establish a failure of the new app. Both V77 and V74
were confirmed running through QMP after boot. V77 has not yet had its persona,
virtual-display, Setup and authentication bootstrap applied, nor has Touch Probe
been launched. UART /tmp/a19-ui-v77-serial.sock, QMP /tmp/a19-ui-v77-qmp.sock,
GDB 127.0.0.1:63477.
