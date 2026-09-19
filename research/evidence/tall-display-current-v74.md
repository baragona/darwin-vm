# Settings artwork renders at the larger display size

A later capture of the same V74 guest at 832x1808 now visibly shows the Settings
gear at the larger icon size. The immediately preceding tall-display-home-v74.png
showed a gray placeholder. No image, scale, or renderer result was substituted
for this follow-up: the command only woke, captured, packed and exported the display.

The UART command completed its SERIAL_DONE marker. The display reported on,
the render returned 1, and the exported frame passed zlib and exact byte-length
validation: 6,017,024 BGRA bytes, 1,504,256 opaque pixels. SHA256:
99e0dfa82967348eabddbc78f20cd3cc6078bbabf1ca350144886e33a505964c.
The decoded PNG was visually inspected and contains actual gear artwork.

Together with the earlier 208x248 logical-window measurement and quarter-scale
icon observation, the larger-mode experiment supports incorrect display geometry
as the cause of the tiny presentation. The larger logical window has not yet been
measured directly. Artwork delivery is delayed; the exact trigger and latency
remain unknown. This is evidence that bitmap artwork renders in the live home
screen, not proof that all image paths or graphical effects work. The dock and
search areas remain solid white. Settings application launch remains unsuccessful.
The geometry adjustment is still a runtime experiment, not a persistent fix.
