# Settings artwork decodes and renders in the guest

Following the successful synthetic bitmap experiment in V75, the new
`software-render-probe --image PATH` mode loads an image through the guest's
ImageIO framework, assigns the resulting CGImage to CALayer.contents, and
renders into the same guarded64x64 software destination. It bypasses
IconServices and remote display-server transport.

## Verified result

The installed `/Applications/Preferences.app/Settings60x60@2x.png` was verified
in the test image against SHA256
37aee99c34b04a870c18f43edcc79f6fffc5a1c59d503ec5448b26296ac9ac03.
The separate V76 guest completed this command:

```
/bin/software-render-probe --image /Applications/Preferences.app/Settings60x60@2x.png
```

ImageIO returned a120x120 CGImage. The local layer commit and render completed,
with16332 changed bytes and zero changed guard bytes. Output length was16384
bytes; the probe exited0 and the UART terminal marker was received.
Changed-byte count measures differences from the0xa5 sentinel, not rendered
pixel coverage: valid image bytes can equal that sentinel.

The exported, zlib-validated64x64 BGRA output is fully opaque and has SHA256
9084de799a0a84f24abdba3540f42f77f24de255ff6532e42819bf318b002513.
The decoded PNG was visually inspected and clearly contains the Settings gear
artwork. This is stronger evidence than merely obtaining a nonnull CGImage.

Evidence:
- settings-image-decode-v76.txt: guest execution
- settings-image-export-v76.txt: lossless UART export
- settings-image-v76.png and settings-image-v76.json: inspected output
- image-decode-build-v76.json: probe, icon, image and trustcache identities
- image-decode-boot-v76.txt: isolated boot; generic thread diagnostics were
  denied with developer mode disabled, while the image probe needed no such
  entitlement and completed normally

## Implication and limits

The installed Apple CgBI PNG is decodable in this guest and can be drawn as
CALayer image contents through the local software compositor. A host image
viewer rejecting that original PNG does not diagnose a guest decoding failure.
This narrows the gray Settings placeholder investigation toward the icon-service
resource/metadata path, asset selection, and remote image/display composition.
It does not prove Assets.car decoding, the icon service's chosen rendition,
remote texture transport, or SpringBoard's final rendering. The quarter-scale
view behavior is still a separate unresolved finding.

No working Settings launch or usable graphical emulator is claimed. The original
V74 UI guest was preserved and independently reported running after the V76 test
VM accepted an intentional QMP quit and its process exited0.

## Build and repeat

Use the same clang invocation as software-bitmap-v75.md, changing the output name
to /tmp/a19-software-image-v76, then ad-hoc sign and strictly verify it. Install
as /bin/software-render-probe in a cloned image and include the actual CDHash in
its trustcache. This build added one entry, for4416 total. No special probe
entitlements were used. The output is /private/var/tmp/software-image.raw;
rendering scales the decoded image to64x64. The --image mode reports decode and
execution failures, but exit0 by itself is not a pixel-content assertion: export
and inspect the result as done here.
