# Settings launch timeout after biometric service restoration (V73)

The guest com.apple.pearld lookup initially returned1102. SpringBoard was stopped
and the already-trusted matching biometrickitd was uploaded to the guest's
/var/tmp/biometrickitd-v72. Its SHA256 is
91328f1a2e6ea8bb84f8e05502d9bb05dfbad88fe003d5f30ec8aa17467efac1.
The existing research/biometric-probe.plist was uploaded as
/var/tmp/biometric-probe-v73.plist; its SHA256 is
cf3a999569e016617b606e677e965ccb3171684f4bc908954a4a13de8d0c4a75.
Both transfers completed hash verification and publication markers. Service
PID92 started, and com.apple.pearld lookup returned0 with port2819.
This is a volatile live installation; it does not add biometric hardware.

Fresh SpringBoard PID95 started with the same Setup/authentication overrides.
The verified extensionkitservice.xpc stat result again had UID99 and mode0755;
only its returned UID at0x7a6d5346b0 was changed to0, then the observer was removed.

The next Settings request completed with LS_OPEN_RESULT=0 and an intact serial
completion marker. A read-only breakpoint at runtime0x186f20528 (unslid
0x186f1c528, the _LSServer_OpenApplicationCommon completion block) observed:

- success x1=0
- NSError x2=0x73dcd57300
- code at+0x10=60
- domain at+0x18 points to NSPOSIXErrorDomain
- userInfo contains LSDispatchUtils.mm and _LSDispatchWithTimeout
- LR0x186f4e93c maps to the SpringBoard-call completion path

The error had already been created when the breakpoint stopped. The pause
therefore cannot explain this particular timeout. It still does not identify
which downstream operation failed to reply. The completion breakpoint and an
unused client callback observer were removed before continuing. No return value
or error object was modified.

Afterward, thread-probe sampled SpringBoard95 (20 threads) and lsd46 (2 threads),
with successful resume results. SpringBoard's main stack was the CFRunLoop /
GSEventRunModal / UIApplicationMain path. LaunchServices' main stack was also a
run loop. Earlier asynchronous debugger samples found SpringBoard executing
_finalizeStartupAfterScenesDidConnect:. This makes startup timing a hypothesis,
not proof of the cause. No Settings process or rendered Settings UI is verified.

A later unpaused retry hit the probe's 30-second alarm while querying the app
record, before LS_OPEN_BEGIN. This is a distinct observation from the completed
launch timeout; do not describe it as another returned launch error. Its trailing
display capture completed with display on, render result1, 206,336 changed and
opaque pixels, and43,918 colored pixels. The command completed its serial marker.
The inspection byte was restored at a verified kernel stop and read back0.

The completed packed-frame export passed zlib and exact825,344-byte validation.
settings-settled-v73.png shows the clock/date, SEARCHING/battery status, two dark
circular controls, home indicator, and “Swipe up to open.” Text is oversized,
clipped, and overlapping in the416x496 surface. This is recognizable lock-screen
content, not Settings. BGRA SHA256:
1fe2860f5794c6800f6a36ae98aadb5a85f5a503ad8fafa5b92e0d23b22f0a8e.

The VM is left running with SpringBoard95 and biometrickitd92. No UART collector
is active. The read-only launch error observers were removed. Existing persona,
credential, backlight, Setup, synthetic frontend-authentication, UI-lock overrides,
and Objective-C exception observer remain. No assertion stop was observed during
this run. Next investigate the lock-screen gesture/display geometry and repeat
launch after verifying SpringBoard's launch service is responding.
