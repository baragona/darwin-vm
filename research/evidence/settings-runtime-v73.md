# Settings registration and first launch attempt (V73)

The prepared Settings image boots to Bash. System app database rebuild returned
1, and a fresh query reports com.apple.Preferences installed at
file:///Applications/Preferences.app/. See settings-registration-v73.txt. This
proves registration, not graphical launch.

The guest runs with settings-v73.tc, a separate version-1 trust cache retaining
all 4,410 original entries plus the new launch probe. ls-open-probe-v73.json
records the executable identity. The probe adds an explicit --open BUNDLE_ID
mode using LSApplicationWorkspace.openApplicationWithBundleID:, with selector
availability checks and a 30-second alarm. Return true would still require
independent process, screenshot, and input verification. Build passed
-Wall -Wextra -Werror; strict signature verification and shell syntax checks pass.

V73 cache slide is 0x4000. UserManager PID38 base is 0x10454c000, derived from
thread-probe return 0x104555980. The manager global at 0x104633398 points to
0x781ec1c040; state+dictionary traversal yielded mobile NSString0x781f00c3c0.
Its text at +0x20 matches CE7B39AA-4774-5BA4-BD1C-A29AB22DB754. The other key
was the all-F system UUID. settings-startup-v73.lldb records this boot's temporary
persona/display/credential/backlight/Setup/authentication overrides. Display
bootstrap breakpoints3/4 were removed after backboardd restarted as PID54.

The developer inspection byte was restored to0 and read back at a kernel stop.
Its kernel pointer at0xfffffe0027ea1600 was read as0xfffffe0017088db4. A first
attempt to restore from a user address space failed; the successful write and
readback occurred at kernel instruction0xfffffe002ad1eb5c.

Launchd base0x1043f8000 was derived from LR0x10441221c and verified by Mach-O
magic. Its stat check0x104421d0c observed extensionkitservice.xpc with helper
result1, directory mode0755, UID/GID99. Only the returned stat UID at
0x7a6d539970 was changed to0. That observer was then removed. PassKitCore's
XPC service was also observed with UID99 but not changed; its rejection may
need separate investigation. Restored framework resources can include XPC bundles.

SpringBoard started as PID69 with Setup/authentication overrides armed. The
first --open com.apple.Preferences returned LS_OPEN_RESULT=0. No successful app
launch or Settings pixels have been verified. Debugger pauses overlapped UART
command entry; the trailing list/marker was damaged (bash reported an invalid
path), so it is not valid process-state evidence. Recover the shell after its
collector ends, then repeat the launch after the graphical services settle.
The guest remains available at QMP /tmp/a19-ui-v73-qmp.sock, UART
/tmp/a19-ui-v73-serial.sock, and GDB127.0.0.1:63473.

Biometrickitd was live-uploaded only on V72 and is not installed in this image.
Restore that validated service before concluding that a repeated launch failure
is caused by Settings itself. A usable interactive graphical emulator remains
unverified.
