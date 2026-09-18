# Recognizer identity and repeated-swipe limits (V72)

A completed virtual swipe with the identified recognizer pointer as a condition
produced zero hits at -[UIGestureRecognizer setState:] (unslid0x184926e9c,
runtime0x187762e9c). The observer was removed. This does not cover internal
state changes that bypass that method.

Two further completed swipes produced zero hits at the previously observed
SBCoverSheetSystemGesturesDelegate touch-acceptance method. The second explicitly
woke the display immediately before input. The wake probe reported display on
both before and after, and the input probe reported14 matching callbacks and its
completion marker. Thus display sleep alone does not explain this particular
missing callback. This also means earlier acceptance cannot be assumed for
all subsequent swipes. The observer was removed.

## Object metadata and state

A one-shot breakpoint at UIKit's MKBGetDeviceLockState return (0x187d6e2e8)
provided SpringBoard's memory mapping for read-only inspection. The recognizer
0x7992471180 has class0x273c5ae30. Its class data resolves through
0x72220bc540 to class-ro0x27689ba88, whose name pointer0x1fdddeeb9 reads
SBCoverSheetScreenEdgePanGestureRecognizer.

The state slot at object+0x48 (0x79924711c8) contains zero. Disassembly of
-[UIGestureRecognizer state] shows the +0x48 storage is used when its feature
flag at unslid0x1e6f7ffc0 is not one. The correct runtime flag address
0x1e9dbbfc0 was read as zero, validating this interpretation for this guest.
(The initial exploratory flag read at0x1e9dbefc0 was a wrong address and is not
used as evidence.) This snapshot indicates the recognizer's possible state,
not a captured failure transition or proof the object is still actively attached.

## Concurrent keyboard activity

At the lock-state return observer, w0 was again0xfffffff3 (-13). A second
one-shot sample had callers0x1886524f8,0x1877c2944,0x187768d08 followed by
notification delivery frames; the first is the previously identified
UIDictationController.dictationIsFunctional path. Repeated AKS17 failures also
continue in UART output. No lock-state result was changed.

The next investigation should separate gesture attachment/routing from the
recurring dictation availability work. A once-idle main-thread sample does not
establish ongoing responsiveness, and a zero-hit gesture breakpoint does not
prove authentication rejection.

All observers added in this experiment were removed or one-shot. Guest execution
was resumed; the Setup bypass and prior bootstrap overrides remain. No new
screen capture or usable-home-screen result is claimed. Transcripts are
recognizer-state-v72.txt, recognizer-class-v72.txt, and recognizer-awake-v72.txt.
