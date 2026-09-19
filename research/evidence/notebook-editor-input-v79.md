# Text editor and persistent input work

Reopening Field Notes and tapping(0.55,0.235) through the native helper put a
visible caret at the end of the note. The832x1808 capture did not show an
on-screen keyboard. The initial serial export failed strict base64 decoding;
re-exporting the same compressed framebuffer succeeded with BGRA SHA256
69389a2f01f37f2a26be9ea48d439b65a654ed2212bd59558bd61a7a6d070fc5.
This proves editor focus, not typed text.

Guest logs reported missing com.apple.TextInput and TextInput.preferences.
Inspection confirmed /System/Library/TextInput/kbd exists(224560bytes), but
its launch plist is absent. keyboard-daemon-probe.plist retains this build's
program, user, environment and Mach services; launch-event subscriptions and
Jetsam policy are omitted for the diagnostic domain. The1710-byte plist was
uploaded into volatile /var/tmp and its SHA256 verified before publication:
0015237cc1d43c761f55a45dcfe1c32a41a72ee8dd4efa2693710c2813febc15.
Service startup and text-input delivery require further runtime verification.

The persistent input prototype now cross-builds, signs and passes host-only
stub state tests. It is not installed in V79 and does not yet provide a live
frontend. See ../live-input-README.md for protocol and limitations.

The keyboard job submitted and started with errno0. Its com.apple.TextInput
Mach-service lookup returned0, and launchd spawned kbd116. This establishes
service availability, not keyboard rendering or typed input.

The guest IconState.plist decoded successfully. Contrary to the earlier
inference from first-page screenshots, the notebook is already on Home page2,
alongside TouchProbe and a Utilities folder containing com.apple.calculator.
Page1 only contains Settings. This points to page navigation, not manual icon
insertion, as the next test. See notebook-iconstate-v79.json.

The new persistent-input endpoint and keyboard launch plist are staged in
service-root-live-input-v80.dmg with4419-entry live-input-v80.tc. The installed
binary matches its signed build hash. The image is detached; V80 has not booted.
The first mount preserved root ownership and rejected the copy; remounting
that guest-image clone with owners ignored allowed the verified copy.

After kbd registration, native Save and editor taps completed. A fresh
frame decoded with SHA256c9b37b548b2ccc9e33ed5abdbd6091c7eceb7b00e8ea68fbaa00d2f930e39149.
Visual inspection still shows a caret with no on-screen keyboard. Starting
kbd alone therefore did not establish visible keyboard input.

Native Home then horizontal drag(0.85,0.5)->(0.15,0.5) completed with exit0.
The page2 framebuffer's initial serial export was corrupted and rejected.
Re-exporting the saved compressed frame decoded6,017,024 bytes, all opaque,
SHA256a653ca863683c3b1005b31bf0b3fad1b421221e5eaaa4e3d3e7cfdaa813dcdcc.
Visual inspection confirms Utilities, Field Notes and Touch Probe on page2.
The icons are gray placeholders, but the app labels and folder are present.
This corrects the earlier inference that the apps lacked Home-screen icons.
