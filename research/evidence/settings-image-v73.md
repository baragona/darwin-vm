# Settings image preparation (V73)

Setup is bypassed in the live V72 guest using the verified temporary override
recorded in setup-skip-runtime-v72.md. The frontend authentication experiment is
recorded in authenticated-ui-runtime-v72.md. Neither proves a usable home screen.

The live guest lists only Setup.app under /Applications. To provide a real app
for launch and interaction testing, clone service-root.dmg with macOS cp -c to
service-root-settings-v73.dmg, then mount the clone with hdiutil using
-imagekey diskimage-class=CRawDiskImage. The running V72 image is not modified.

Copy /Applications/Preferences.app from the matching mounted system firmware.
Inspect its direct imports with otool -L. For directly imported framework bundles
absent from the clone but present in the mounted system firmware, copy their
resource directories using ditto. This installs 54 bundles in total, with 1,360
files and 180,891,830 logical bytes. Every copied regular file was compared by
SHA-256 against its source; settings-image-v73-manifest.json records the results.
This is a direct-resource restoration, not a complete recursive dependency audit.
Framework code supplied by the shared cache need not have an on-disk executable.

Both executable files in Preferences.app already have CDHashes in the existing
4,410-entry version-1 ramdisk.tc:

- Preferences: 5cf30fe0a11109ab4d111a984beea8b76f33e6f3
- SettingsImportExtension: ce78790a114cd63f9aeba1a31d1fe53512627fe7

No signing or trust-cache changes were required for these two executables.
The clone was unmounted after copying. No Apple firmware files are committed.

Status: image prepared and copy integrity verified; it has not yet been booted.
Next: boot this image, reapply bootstrap and Setup/authentication overrides using
the new boot's addresses, rebuild LaunchServices, query com.apple.Preferences,
and attempt a foreground launch with screenshot and input verification. Settings
may expose further missing services or resources. Do not report it as running yet.
