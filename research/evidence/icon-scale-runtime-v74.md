# Runtime icon scaling and unresolved artwork delivery (V74)

With IconServices51 restored, the system-app rebuild returned1 and a fresh display
query verified LCD416x496. The first display query aborted during render-server
startup; the later query completed. MobileGestalt80, biometric83, RunningBoard86
and SpringBoard89 were started (consult the transcripts for exact launch state).
The known extensionkitservice stat check was verified at launchd0x102099d0c,
base0x102070000 (Mach-O magic checked). Path, helper result1, mode0755 and UID99
were observed before changing only the returned UID at0x75194111f0 to0. That
observer was removed. Setup/authentication bootstrap overrides remained active.

The first capture stayed display-off with zero changed pixels; its buffer is not
rendered UI. The next wake sequence reached display-on but captured opaque black.
These are startup observations, not evidence that all image rendering fails.

## Settings icon receives quarter-scale values

A bounded read-only observer at0x1cdb006f0 (unslid0x1c4fec6f0,
-[SBIconView _applyIconContentScale:]) recorded:

- self0x0800007d4b5f3000: d0=1, then0.24999999999999967
- self0x0b00007d4b6a6a00: d0=1, then0.24999999999999967, then0.95,1,0.95

Both views were identified independently as Settings. Runtime ivar metadata at
0x1ee92b7b4 gives SBIconView._icon offset0x1a0. Both views refer to icon
0x7d4b4055c0. SBLeafIcon.leafIdentifier offset0x60 points to NSString0x1f1882430,
whose constant-string bytes at0x1894c39ff read com.apple.Preferences.

No scale argument was changed. These observations show real quarter-scale
requests for Settings views, not a proven persistent transform on the particular
visible icon. Two views share one model and one later returns to full scale.
Transitions or different presentation contexts remain possible. A later attempt
to read their current scale fields failed due to the debugger's current address
space; it supplies no evidence of current field values. A proposed one-shot
SpringBoard inspection breakpoint did not fire and was removed. Both scale and
inspection observers were removed before leaving the guest running.

The defaultIconImageSize50 and defaultIconImageScale2 constants documented in
icon-artwork-v74.md are distinct from these live content-scale observations.
Do not conflate bitmap backing scale with view size or content transform.

## Icon service is waiting for application metadata

The completed thread sample resumed SpringBoard89 (18 threads) and
IconServices51 (3 threads) successfully. SpringBoard's main thread was in its
CFRunLoop/GSEventRunModal path. An icon-service worker was waiting in:

NSXPCDistantObjectSimpleMessageSend1 -> _LSCopyServerStore ->
_LSContextInitCommon -> LSApplicationRecord.initWithBundleIdentifier:fetchingPlaceholder:error:
-> ISBundleIdentifierIcon._makeResourceProviderAllowIconResourceFallback:
-> ISGenerationRequest.generateImageReturningRecordIdentifiers:.

Selected runtime/unslid frames:

- 0x18f90b4c4 /0x186df74c4: _LSCopyServerStore+764
- 0x18f8ea37c /0x186dd637c: LSApplicationRecord initializer+52
- 0x1ca359aa8 /0x1c1845aa8: icon resource-provider lookup+76
- 0x1ca3428ac /0x1c182e8ac: icon generation+564

This is a snapshot of a blocked metadata request, not proof that it never
completes or that it is the sole artwork failure. It connects the missing-icon
investigation to the earlier slow app-record queries. It does not establish a
GPU texture-rendering defect.

## Visible result: unchanged artwork

After wake and14 matching swipe callbacks, iconservice-visible-v74.txt completed
its capture/export marker. The display was on, all206,336 pixels changed and
were opaque, and98,883 were colored. Zlib/frame validation passed. The decoded
PNG is byte-for-byte the same BGRA frame as settings-lock-swipe-v73.png:
5be3e1e097018ef60d93ed6bd4923f08edfdf75a284fb2e4253b127725f18a7c.
It still shows a tiny gray Settings icon, small label, status bar and oversized
white shapes. Restoring the service alone did not visibly fix either symptom.

The new dimension-aware capture and file-length-aware packer have now run in the
guest at416x496 and produced the expected825,344-byte frame. Taller modes remain
untested. The inspection flag was restored and read back0 after sampling.
Next trace the LaunchServices store request and the caller that requests quarter
scale; separately test bitmap-backed layers if artwork delivery succeeds but
pixels remain absent. No working Settings launch or usable GUI is claimed.
