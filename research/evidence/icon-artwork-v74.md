# Investigating tiny and blank Settings artwork

The user's screenshot distinguishes two problems: the Settings icon/label are
small relative to the status bar, and the icon is a plain gray shape. Neither
symptom is explained merely by increasing the diagnostic display height.
V74 therefore retains the416x496 mode for the icon-service experiment.

Settings Info.plist declares CFBundleIconName=Settings and icon file
Settings60x60. Settings60x60@2x.png is a120x120 Apple CgBI PNG. Assets.car is also
present. Both files were copied into V73 and inherited by V74 with matching
SHA256 values recorded in settings-image-v73-manifest.json. The host image
viewer cannot decode this CgBI PNG; that is not evidence that iOS cannot.

V73 logs show both lsd and SpringBoard failing to look up com.apple.iconservices.
The original matching iconservicesagent is134,448 bytes, already trusted by the
guest cache. iconservice-identity-v74.json records its identity. It was uploaded
to /var/tmp/iconservicesagent-v74 with verified SHA256, and a probe job was
prepared from its matching firmware launch plist. The job retains the original
_iconservices user and Mach service, disables pressured exit for observation,
and writes stdout/stderr under /var/tmp. This is a volatile guest restoration.

Separate sizing lead: SpringBoardHome's +[SBIconView defaultIconImageSize]
(unslid0x1c4f9ebb0) returns50x50, and defaultIconImageScale (0x1c4f8ea24) returns2.
The previous virtual-display/HID geometry reported scale1. Image backing scale
is not necessarily view content scale; these defaults do not establish which
values the live Settings icon uses. Observe its actual size/content transform
before claiming that a scale mismatch explains the tiny icon. Relevant methods:
iconContentScale0x1c4f8cfdc, iconImageSize0x1c4f8e42c,
_applyIconContentScale:0x1c4fec6f0, effectiveIconContentScale0x1c51890d0.

Restoring IconServices tests a concrete missing dependency. If artwork remains
blank, independently test bitmap-backed CALayer contents before concluding that
all images fail in the compositor. Existing solid-color/text screenshots alone
do not prove that image decoding, image delivery, and texture rendering work.

## Runtime restoration checkpoint

The executable and probe plist uploads both completed hash verification.
IconServices PID51 starts and its Mach endpoint lookup returns0 (ports3587,
then2819 on a fresh query). Logs confirm execution as iconservicesagent. The
expected stderr file did not exist at the early check; this is not proof of
error-free operation. MobileGestalt and posterboard-service lookups were missing
at this stage, before the remaining UI support services were started.

V74 runs at QMP/tmp/a19-ui-v74-qmp.sock, UART/tmp/a19-ui-v74-serial.sock, and
GDB127.0.0.1:63474. Cache slide0x8b14000. UserManager54 base0x1003d8000, derived
from thread return0x1003e1980. Global0x1004bf398 -> manager0x7de9024040 ->
state0x100b4d790 -> dictionary0x100b4d7d0 -> storage0x7de901c000. Mobile
NSString0x7de9018280 was verified against the full UUID; the other key is all-F.
Boot-specific bootstrap commands are in iconservice-startup-v74.lldb. The
inspection byte was restored and read back0 before support services started.

One rebuild command was interrupted during serial entry by the persona
breakpoint; its transcript shows only an invalid trailing command. After its
collector timed out, shell recovery completed with GUEST_READY and its serial
marker. No reboot was performed in response to that observation timeout.

No post-restoration SpringBoard screenshot has yet verified icon artwork.
Keep the missing-service finding distinct from a demonstrated visual fix.
