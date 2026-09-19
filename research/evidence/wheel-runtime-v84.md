# Guest-timed wheel gestures (V84 staged)

Protocol4 adds S x0 y0 x1 y1. It preallocates all14 parent/finger events before
pressing and refreshes timestamps immediately before each dispatch, with40ms
between frames. A mid-gesture dispatch failure retains the last acknowledged
touch state for explicit or idle release. Invalid coordinates, extra fields,
and a swipe during an existing contact dispatch no events.

The viewer maps vertical wheel input to vertical swipes and horizontal or
Shift+wheel to horizontal swipes, throttled to one per650ms. Older agents do
not receive S. Pointer dragging remains available for freehand drawing.

C input-state tests, all five Python protocol tests, and the actual browser
handler tests pass. Strict iOS cross-compilation and signature verification pass.
The signed binary was installed and hash-verified in a clone of V83; its CDHash
is present in wheel-v84.tc (4424 entries). The writable image was detached.
This image has not booted; wheel navigation/scrolling are not runtime verified.
