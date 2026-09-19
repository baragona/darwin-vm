# Home navigation in V78

Starting state: TouchProbe87 visible with Taps: 1, SpringBoard84 and BackBoard80.
The synchronous hosted-layer workaround remains enabled.

The existing --virtual-swipe completed with exit 0. A strict decode of the
fresh capture produced SHA-256
4daf4bfdd9be1ade377592840c3325393d51ba8c7729c329c36195686b0d9353,
identical to the preceding Taps: 1 app frame. It did not return Home.

The existing --home command sends Consumer-page Menu usage 64 down and up.
It completed with two monitor callbacks and exit 0. A fresh capture rendered
all 1,504,256 pixels opaque and a different colored-pixel count. Its first
serial export failed zlib validation, so that transfer is not visual evidence.
Re-export of the same saved frame is being inspected separately.

The new --virtual-drag command is source-only pending installation. Strict
arm64 iOS compilation passed; 33 parser cases exercised valid endpoints and
invalid coordinates with a dlopen stub that prevents host HID calls. These
checks do not prove guest drag delivery or Home recognition.

The re-export decoded successfully: 6,017,024 BGRA bytes, all 1,504,256 pixels
opaque, SHA-256 02997a1eb162d800731ed3aec430d98128ceee116900a19948ff3a225a588742.
Visual inspection confirms SpringBoard Home with a Settings icon, empty dock,
and no Touch Probe icon. Settings has a gray placeholder and the dock/search
backgrounds are white; graphical completeness is not established. This
verifies Home-key navigation, not icon-based app launching. LaunchServices
reopen is being checked separately.

LaunchServices then returned LS_OPEN_RESULT=1 and REOPEN_EXIT=0. The fresh
post-reopen capture decoded all 6,017,024 bytes with 1,504,256 opaque pixels,
SHA-256 4daf4bfdd9be1ade377592840c3325393d51ba8c7729c329c36195686b0d9353.
Visual inspection confirms the app is foregrounded again with Taps: 1.
This proves state retention across Home and reopening of the running app.
It does not prove durable storage, process-relaunch persistence, or launching
from a Home-screen icon. The abort observer still contains only the earlier
pre-workaround assertion; no new abort was recorded in this sequence.
