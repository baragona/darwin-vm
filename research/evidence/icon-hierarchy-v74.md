# Tiny icon belongs to the home screen; logical window is208x248

A read-only snapshot identifies the two Settings presentations and their layer
ancestry. See icon-hierarchy-v74.json for all sampled fields.

- View0x7d4b5f3000 is SBIconView, with previously verified content scale0.25.
  Its backing layer is56x70, positioned at(27,30) inside SBIconListView.
  Its ancestry includes SBIconScrollView, SBRootFolderView, SBIconContentView,
  SBHomeScreenView and SBHomeScreenWindow. The home-screen window and list bounds
  are208x248. This is a home-screen icon, not merely an app-switcher preview.
- View0x7d4b6a6a00 is SBHLibraryCategoryPodIconView, with scale0.95. Its ancestry
  includes SBHLibraryCategoryPodIconListView, _SBHLibraryPodIconView,
  _SBHLibraryPodIconListView, and SBHLibraryPodFolderView. The pod list is320
  points wide inside208-point-wide ancestors. Its root-folder layout wrapper
  has position(520,124), whereas the home icon list has position(312,124).
  Both join the same SBIconScrollView with bounds origin(208,0) and size208x248.
  The second chain was capped at20 layers, so it is a partial ancestry snapshot.

The physical capture is416x496 pixels. The observed208x248 logical home-screen
bounds establish the two-to-one relationship for this window. The narrow logical
screen, quarter-scale home icon, and oversized neighboring library content make
display geometry the next useful experiment. This is not yet causal proof that
increasing resolution fixes scaling or clipping. No bounds or scales were changed.

## Read method and corrections

UIView._layer's runtime ivar metadata at0x1ecb00994 yielded offset0xb0. CALayer's
implementation pointer is at+0x10. Exact-build getter disassembly supplies bounds
at implementation+0x68 and position at+0x58; the custom-transform flag is bit6
of byte+0x38. Class names were matched against class addresses from this firmware's
UIKitCore, QuartzCore, SpringBoardHome and SpringBoard symbol lists, with cache
slide0x8b14000. Unknown classes remain explicit ISA addresses in the JSON.

The first ancestry attempt incorrectly treated implementation+8 as a parent
CALayer object. It is a parent implementation pointer; its+0x10 is the parent
CALayer. That attempt hit unmapped memory and yielded no valid hierarchy. The
corrected walk produced the saved snapshot. Delegate pointers were read from
implementation+0x88; these are a stopped-process diagnostic, not retained objects.
The two starting views were gated on the already verified Settings _icon pointer.

The original LLDB session exited during interrupt/recovery of the unsuccessful
callback attempt. QMP reported the guest running. A new debugger connected to the
same guest on port63474, which stopped at the prior run-loop address. The existing
persona, credential, backlight, Setup, frontend authentication and UI-lock
bootstrap observers were recreated, along with the exception observer. An explicit
attempt to remove the old run-loop remote breakpoint returned E22; no removal is
claimed from that response. The new diagnostic breakpoint was then created,
used for the corrected stopped-context read, deleted, and the guest resumed.
The successful hierarchy read was performed directly while stopped, avoiding
repeated automatic callbacks on unmapped process contexts.

Current debugger session uses breakpoints1/2 persona,3 credential,4 backlight,
5 Setup,6 frontend authentication,7 UI-lock and8 exception. No hierarchy observer
remains active. No guest scale, layer pointer, bounds, or image content was
modified by this experiment.

## Next experiment

Test a taller, wider virtual display while retaining the existing scale behavior;
832x1808 pixels is a diagnostic candidate for a416x904-point layout if the observed
2:1 mapping persists. Verify actual CADisplay mode and SpringBoard window bounds
rather than assuming that mapping. Use the dimension-aware capture and decoder.
Keep the prior image and captured baseline available for comparison. This is not
a claim about the hardware's native resolution or a verified fix.
