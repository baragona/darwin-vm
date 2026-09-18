# Lock-screen swipe and taller-display preparation

The V73 wake/swipe/capture sequence completed with its serial marker. The
resulting settings-lock-swipe-v73.png shows a small Settings icon at top left,
status bar, and large white rounded shapes. It differs from the clock/date lock
screen. It is evidence of visible progression after input, not a usable launcher
or proof that Settings opened. The BGRA frame has98,883 colored pixels and is
fully opaque; SHA2565be3e1e097018ef60d93ed6bd4923f08edfdf75a284fb2e4253b127725f18a7c.

The launch retry afterward was slow in its preliminary app query but eventually
confirmed installation, sent the launch request, and returned LS_OPEN_RESULT=0
with an intact completion marker. An earlier retry had hit the alarm during
that query. To isolate those operations, --open now only sends the launch
request; --query remains the separate registration diagnostic. This change is
compiled and staged, not yet exercised in the guest.

QuartzCore's built-in virtual-main-display constructor supplies416x496 constants
in its options dictionary at unslid0x18475d46c. These are diagnostic dimensions,
not an established phone display configuration. A taller mode is the next
experiment; it has not been applied yet. The oversized/overlapping UI may have
other causes, including scaling, assets, and unfinished transitions.

Capture tooling now reads width/height from CADisplay.currentMode (64-bit getters
verified in this firmware), bounds the frame to16MiB, and captures that size.
The packer reads the bounded file length rather than assuming416x496. The decoder
accepts --width and --height, retaining416x496 defaults for old evidence. Exact
frame-length and zlib-completion checks remain. A synthetic416x904 test decoded
successfully and was rejected when decoded with wrong legacy dimensions; the
existing V73 frame decoded to the identical hash. Both guest C probes and the
direct launch probe build with-Wall -Wextra -Werror and pass strict signature
verification. Runtime validation of the new binaries is still pending.

service-root-geometry-v74.dmg is an APFS clone of the V73 image. It contains the
three rebuilt probes and the original trusted biometrickitd at/usr/libexec, plus
/launchjobs/com.apple.biometrickitd.probe.plist pointing there. This avoids the
next boot's slow UART daemon upload. All four binary copies were hash-verified;
geometry-image-v74.json records identities. The separate geometry-v74.tc retains
all4411 V73 entries and adds three probe hashes, for4414. The image was unmounted
cleanly and has not yet been booted. V73 remains running with its old tools.

Next boot with RAMDISK pointing to service-root-geometry-v74.dmg and TRUSTCACHE
to geometry-v74.tc. Re-derive all debugger addresses and apply the established
bootstrap and Setup bypass. Change the virtual LCD dimensions at construction,
verify CADisplay mode and HID geometry, then capture/export with explicit matching
decoder dimensions. Neither a new phone resolution nor a working app launch is
claimed by this preparation.
