# V83 short taps and Home resources

V82 is closed after preserving its terminal scheduler panic. V81 was resumed
through LLDB and the existing note/drawing state remains in that guest. A fresh
viewer validated Home frames; a one-move browser swipe did not expose the
Notebook icon. Six frame rejections were observed amid interleaved daemon console
output before a fresh complete frame recovered. App-icon reopening is unverified.

Read-only mounting of the closed V82 image confirmed that
System/Library/PrivateFrameworks/SpringBoardHome.framework is entirely absent,
although SpringBoard.app/SpringBoard.loctable is present. The matching firmware
framework's English table maps REMOVE_APP_SHORTCUT_ITEM_TITLE to Remove App.

V83 clones V82 and restores all45 files from the matching SpringBoardHome
framework, each byte-compared and SHA256-recorded in taps-staging-v83.json.
It installs the strictly cross-built and ad-hoc-signed protocol3 input agent;
the version1 trust cache now has4423 entries and includes its verified CDHash.
The writable mount was detached after verification. No Apple firmware binaries
are included in git.

The agent preallocates a tap's down/up events, refreshes both parent and finger
timestamps immediately before dispatch, and waits40ms between tap dispatches.
A failed release retains held state so R/idle cleanup can retry. Browser tests
exercise real handlers for quick clicks, drags, holds, cancellation, and older
agents. C state tests and all four Python protocol tests pass. This is prepared
for runtime validation, not yet evidence of Calculator arithmetic or reliable
Home-icon reopening. No kernel timing assertions have been bypassed.
