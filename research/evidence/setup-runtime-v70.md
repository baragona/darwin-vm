# Setup restoration on iOS 27.0 (24A437)

V70 boots the enlarged 9 GiB APFS root with 16 GiB guest memory. The original
Setup.app and SetupAssistant.framework were restored from the matching system
image; all 1,142 regular files match their source hashes and modes. Both Mach-O
executables were already in the guest trust cache. See `setup-image-v70.json`.

The diagnostic `../setup-daemon-probe.plist` retains the original nine Mach
services, executable, mobile user, and adaptive spawn type. It omits background
LaunchEvents and adds explicit diagnostic output paths. Register and start it:

```sh
/bin/launch-probe submit /launchjobs/com.apple.purplebuddy.budd.plist
/bin/launch-probe start com.apple.purplebuddy.budd
/bin/launch-probe lookup com.apple.purplebuddy.budd.xpc
```

budd stayed at PID 39 with LAST_EXIT=0. The final service lookup returned RESULT=0
and PORT=6659 (`setup-budd-lookup-v70.txt`). The job label without `.xpc` is not a
Mach service name: looking that up returned 1102. LaunchServices rebuild returned
1. This does not yet establish that Setup can launch or complete onboarding.

## Disk growth caveats

The 8 GiB image had only about 40 MiB free. Copying Setup failed with ENOSPC; that
incomplete image was never booted. Preserve a copy-on-write backup before edits.
For this verified whole-disk raw APFS image, restoring the backup, extending the
raw file from 8 to 9 GiB, then attaching and running `diskutil apfs resizeContainer`
against its identified container succeeded. Its filesystem check exited 0.
Do not apply this recipe blindly to other disk-image formats or host containers.

Resize remounted the guest volume under `/Volumes/a19-ui-probe`, abandoning the
requested `/tmp` mountpoint. Reattach and verify `df` and the guest device mapping
before copying: an old mount directory can silently become an ordinary host
directory. Final installation was verified on the guest volume before detach.

## Next failure: keyboard label

The only completed display capture was black. An exception interrupted the next
planned gesture experiment; its empty transcript does not prove a swipe executed.
At objc_exception_throw, the exception was NSInvalidArgumentException, reason
`attempt to insert nil object from objects[0]` in a dictionary initializer.

With cache slide 0x9aa4000, frame 2 maps to unslid 0x18507c82c in
`-[UIKeyboardImpl updateReturnKey:]`. Its objects array contained zero for
`UITextInputReturnKeyStateChangedDisplayStringKey`. The producer is
`returnKeyDisplayName`; static inspection follows it into keyboard localization.
Allowing the exception to proceed was followed by SpringBoard SIGABRT and repeated
restarts. No exception suppression was installed. The SpringBoard job was removed
to stop that loop; the guest was subsequently stopped for an offline update.

Guest `ls` confirms these four directories are absent:

- `/System/Library/PrivateFrameworks/TextInputUI.framework`
- `/System/Library/PrivateFrameworks/TextInput.framework`
- `/System/Library/KeyboardLayouts`
- `/System/Library/TextInput`

All four exist in the matching original firmware. Restore and verify their files
for V71, then repeat startup and the input experiment. Their absence is a concrete
dependency gap, but their causal relationship to the nil label remains unproven.
The UI still requires the documented diagnostic bootstrap overrides; this is not
yet a usable graphical emulator.

V71 preparation is complete: 1,464 files (14,324,775 logical bytes) were copied and
hash/mode verified, with symlinks checked. The two included Mach-O files, kbd and
MessagesDataKeyboardPlugin, already have entries in the unchanged trust cache.
See `keyboard-image-v71.json`. The guest image was backed up as
`service-root.pre-keyboard-v71.dmg` and detached after verification. No V71 runtime
result is claimed yet. Mounting with `-owners off` allowed offline copying into
the guest's root-owned directories without changing host permissions.
