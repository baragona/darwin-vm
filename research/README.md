# A19 / iOS 27 graphics investigation

Goal: a usable graphical A19/iOS 27 guest. This experiment establishes a boot
framebuffer and UART input path; SpringBoard, touch/HID, accelerated graphics,
and a full installed system are not working yet.

## Current checkpoint

- The full iOS shared cache maps using the documented diagnostic owner-check patch.
- Backboardd starts under launchd with its original mobile identity and MachServices.
- Notifyd and both preference daemons start through normal boot registration.
- HID, render, notification and preference Mach-port lookups succeed.
- `IOSurfaceRoot` is registered, but no GPU attachment or rendered UI is verified.
- LLDB works through QEMU; a temporary TXM state override enables guest task inspection.
- The AppleKeyStore endpoint-order patch gets backboardd past its initial wait.
- Native tmpfs mounting at `/private/var` works with the original mount helper.
  The earlier `/mnt1` symlink and container-helper entitlement experiment are
  superseded; the original container helper is restored in v30.
- Matching ICU data gets container migration past date-formatter initialization.
- Backboardd reaches its main run loop and QuartzCore render-server threads.
- A fresh CADisplay query completes: five wireless displays, all with 0 × 0
  modes, and no main display. A usable output and rendered UI remain absent.
- SpringBoard and graphical interaction remain unfinished.

## Verified milestone (2026-09-17)

iPhone18,3, iOS 27.0 build 24A437, kernel
`xnu-13432.2.10~2/RELEASE_ARM64_T8150` booted to Bash with text rendered by the
guest kernel into a 1024x768 framebuffer. Commands entered through the emulated
UART executed in the guest and their output appeared on screen:

- `uname -v`: the T8150 kernel above.
- `id`: `uid=0(root) gid=0(wheel)`.
- `echo A19_FRAMEBUFFER_INPUT_OK`: marker visible on screen.

![Guest-rendered console and command results](evidence/a19-console.png)

This is actual guest framebuffer output. The host only copies guest physical
memory into a QEMU display surface. No host-generated text or pattern was used.
The image is a QMP screendump, not evidence of SpringBoard or a GPU driver.

## Reproduce

Use the prepared firmware from SETUP-NOTES.md, including its ownership fix.
Apply this experimental patch to qemu-sptm commit
`2867d847d3471560e773120ee50c42dbcbb6d60b`:

```sh
git submodule update --init
git -C qemu-sptm apply ../research/boot-display.patch
mkdir -p qemu-sptm/build
cd qemu-sptm/build
../configure --target-list=aarch64-softmmu --disable-pvg
make -j8
cd ../..
mkdir -p logs
FIRMWARE_DIR=/absolute/path/to/firmware/iphone-17 \
  SERIAL='unix:/tmp/a19-display-serial.sock,server=on,wait=off' \
  ./research/run-boot-display.sh > logs/boot-display.log 2>&1
```

In another terminal after the guest boots:

```sh
python3 research/send-serial.py /tmp/a19-display-serial.sock 'uname -v'
python3 research/send-serial.py /tmp/a19-display-serial.sock 'id'
python3 research/qmp.py /tmp/a19-display-qmp.sock screendump \
  '{"filename":"/tmp/a19-screen.ppm"}'
```

Allow commands to finish before capturing. The tested boot takes tens of
seconds on this host; elapsed time alone does not prove a shell is ready.
Repeated SEP timeout messages can scroll the results away. `DISPLAY_BACKEND`
defaults to `none`; a GUI frontend such as `cocoa` can be selected in a build
that supports it, but this experiment verified QMP screenshots only. QEMU GUI
keyboard/mouse input is not connected to an iOS HID device. Use UART input.

Stop this experimental VM with:

```sh
python3 research/qmp.py /tmp/a19-display-qmp.sock quit
```

Use distinct QMP and serial socket paths for simultaneous instances. The
standard launchers retain their original behavior; the display is opt-in via
`-M darwin,boot-display=on`.

## Findings and experiments

1. Upstream leaves `boot_args.Video` zeroed. The prototype supplies a physical
   address, 4096-byte stride, 1024x768 dimensions, and 32-bit pixel depth.
2. Simply extending `topOfKernelData` to reserve the framebuffer fails before
   XNU serial output. QMP inspection of SPTM's guest stack recovered:
   `Frame has been left untyped during bootstrap 0x0000010015f84000`.
   The display stayed black. The CPU was still running in EL2, so lack of serial
   output did not mean QEMU itself had exited.
3. The current prototype pads the end of the RAMDisk memory-map range with
   3 MiB for the framebuffer. SPTM then covers these pages with a known range,
   and the same restore filesystem still boots. **This is an experimental
   reservation workaround, not a final framebuffer memory design.** It changes
   the guest ramdisk's reported size. The source DMG is never modified.
4. `serial=3` keeps output on UART; the video surface has essentially only its
   initial cursor. `serial=0` sends console output to the screen. `serial=2`
   additionally enables UART input while leaving screen output selected.
   This follows XNU's `SERIALMODE_OUTPUT=1` / `SERIALMODE_INPUT=2` definitions.
5. Serial input is paced (40 ms per byte) to avoid overflowing the emulated
   UART FIFO. Kernel/service logs interleave with typed commands.

The framebuffer is an early console only: no emulated display controller,
IOGPU user client, Metal, touch device, or normal system-volume boot has been
implemented. The code uses full-frame copies on display updates, a fixed size,
and only the SPTM loader. Reset/migration and other devices are unverified.

## Next investigation

The exact kernel collection has 319 kext entries, including `com.apple.AGXG18P`
(360.34.5a1), `AGXFirmwareKextG18PRTBuddy`, `EXDisplayPipeH18P`, `IOGPUFamily`,
`IOSurface`, and Apple HID/multitouch drivers. Their presence in the collection
does not mean they attach to the deliberately stripped device tree.

The full filesystem image from the same IPSW was extracted locally to
inspect SpringBoard, backboardd, and their launch/service dependencies:

```sh
ipsw extract --remote \
  'https://updates.cdn-apple.com/2026FallFCS/b18c9502-c21d-4555-9bf7-21f3a238e6d7/iPhone18,3_27.0_24A437_Restore.ipsw' \
  --dmg fs --output ipsw_db/iphone-17-full --flat --json
```

The system filesystem does not contain the dyld shared cache. That is in the
separate `Cryptex1,SystemOS` image, `043-69319-704.dmg.aea`, selected by the
same build manifest (`ipsw extract --dmg sys`). Its main cache and subcaches
must accompany the UI executables. The probe copies the decrypted cryptex to
`/System/Cryptexes/OS` and creates the conventional cache-directory symlink.
Neither SpringBoard nor backboardd has reached a working UI.

### Larger userspace probe

A disposable 8 GiB APFS image was created from the original restore ramdisk
using `hdiutil create -size 8g -fs 'Case-sensitive APFS' -layout NONE
-volname a19-ui-probe -srcfolder /tmp/a19-restore -srcowners on -anyowners
-format UDRW ...`. This preserves the existing root-owned launch configuration.
The protected `private/etc/master.passwd` required a separate copy with source
ownership disabled. The system cryptex, backboardd, SpringBoard.app, and ioreg
were then copied into this image. This is a diagnostic ramdisk, not a normal
iOS system/data-volume installation.

The first boot exposed a size truncation: APFS saw only the extra 3 MiB
framebuffer padding, rather than the 8 GiB image plus padding. XNU's memdev
block-count and block-strategy code shifted a 32-bit page count before
widening it. `patch-memdev-block-size.py` changes four instructions in the exact
24A437/T8150 kernel collection to perform those operations with 64-bit values.
The script checks the entire input SHA-256 and each original instruction,
writes a new file, and refuses to overwrite an existing destination. It does
**not** fix raw character I/O or core-dump range reporting. This is an
experimental block-device workaround, not a general memdev patch.

With that kernel, APFS reported 16,783,360 512-byte device blocks, mounted the
8 GiB volume, and reached a verified root Bash shell. Patch hashes and offsets
are recorded in [memdev-patch.json](evidence/memdev-patch.json).

The guest needs 16 GiB configured in both QEMU and the device tree.
`set-guest-memory.py` modifies only the existing `dram-size` property. A full
round trip through dt_fixup is unsafe here: its string heuristic adds a NUL
to the already-patched all-`A` random-seed, changing 256 bytes to 257 and
causing an early SPTM panic.

The probe trust cache merges the baseline hashes with those from the system
and cryptex trust caches (4,372 unique SHA-256 CDHashes). The current generator
emits version 1 and discards version 2 constraint metadata; this is a research
limitation. Firmware and Apple's binaries remain local and untracked.

In the first full probe, dyld could not map the shared cache. ioreg and
backboardd exited 134, reporting missing libraries that should come from that
cache. Starting SpringBoard produced repeated TXM errors (`selector: 38 | 42`)
and the experiment was stopped. Cache ownership and mapping must be resolved
before interpreting these failures as graphics-driver requirements.

The writable-remount experiment failed: mount_apfs returned "Operation not
permitted" (77), and chown failed because the guest root stayed read-only.
A subsequent guest stat reported the cache owner as `99:99`, not root.
The read/write host mount with ownership disabled also refused chown to root.
See [probe excerpts](evidence/ui-probe-excerpts.txt).

`cache-diagnostics.c` enables `vm.shared_region_trace_level=4`, verified from
its previous value of 1. No extra shared-region diagnostics appeared on UART;
reading `kern.msgbuf` returned only a few bytes, not a usable log. It can be
built without the iPhone SDK by declaring the few libSystem functions used:

```sh
xcrun clang -target arm64-apple-ios27.0 -isysroot /tmp/a19-restore \
  -nostdlib research/cache-diagnostics.c \
  /tmp/a19-restore/usr/lib/libSystem.B.dylib -o /tmp/cache-diagnostics
codesign -s - /tmp/cache-diagnostics
```

Copy the executable into the probe's `/bin`, add its CDHash to `all_hashes`,
and regenerate the guest trust cache before booting. Only firmware/tool hashes
are included; no Apple binaries are committed.

`ui-probe.sh` runs this diagnostic, reports ownership, and tries ioreg/backboardd;
SpringBoard is omitted until those prerequisites work. `run-serial-probe.py` waits for the Bash prompt,
sends the probe with paced UART input, and records a transcript. A timeout
leaves the VM alive for inspection; it is not evidence that the VM stopped.

[Device inventory](evidence/display-device-inventory.json) compares the original
and patched matching properties: DCP, EXDisplayPipe and related devices lose
driver matching, and the GPU node is removed. The kernel collection contains
relevant drivers, but they cannot be assumed to attach. The cache-mapping experiment and subsequent service investigation are recorded
below.

A separate diagnostic kernel (`patch-cache-owner-check.py`) changes only the
conditional branch rejecting a nonzero `va_uid` to NOP, on top of the verified
memdev patch. It is pinned to that exact input hash. This deliberately weakens
the experimental guest's shared-cache admission and must be replaced by proper
image ownership for a maintained implementation. It tests whether the known
metadata mismatch explains the loading failure; it does not disable the other
mapping checks. [Patch record](evidence/cache-owner-patch.json).

## Sources

- [XNU boot video initialization](https://github.com/apple-oss-distributions/xnu/blob/main/pexpert/arm/pe_init.c)
- [XNU video console](https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/console/video_console.c)
- [XNU serial mode definitions](https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/console/serial_protos.h)

These public sources informed the experiments; the actual behavior above was
verified on the requested iOS 27 guest. Local logs and firmware stay untracked.


### Shared cache mapped; backboardd reaches service initialization

The controlled owner-check experiment succeeded: launchd's dyld printed
`dyld cache mapped system-wide: customer, auth GOTs: unmapped`. No
"syscall to map cache into shared region failed" messages appeared in that
boot, ioreg exited 0, and backboardd advanced into service initialization.
The IOAccelerator query returned no devices, so exit 0 does not establish a
working GPU. The guest still reported owner `99:99`; the single branch patch,
not a metadata correction, enabled this result.

Backboardd then requested missing notification, preference, RunningBoard,
MobileGestalt, and other services. Its shell launch entered a user/501
bootstrap context despite uid 0, and the sandbox denied registration of
`com.apple.iohideventsystem`. It also spawned ACCHWComponentAuthService through
XPC. This is evidence of executing UI-service code, not of a working compositor.
The next controlled test should use launchd with the real MachServices
registration and appropriate service dependencies. The source backboardd plist
also declares a `_Conclave`; the restore launchd reports that it skips
`init-exclavekit`, so that path remains an explicit unknown.

Reproduce the latest probe after preparing the local image/trust cache:

```sh
FIRMWARE_DIR=firmware/iphone-17-ui-probe \
RAMDISK=firmware/iphone-17-ui-probe/ui-root.dmg \
BOOTKC=firmware/iphone-17-ui-probe/bootkc-cache-owner-probe MEMORY=16G \
QMP_SOCKET=/tmp/a19-ui-qmp.sock \
SERIAL='unix:/tmp/a19-ui-serial.sock,server=on,wait=on' \
BOOT_ARGS='rd=md0 serial=3 -v -noprogress wdt=-1 wlan-olyhal-abort' \
./research/run-boot-display.sh

# Second terminal; transcript path must not already exist:
python3 research/run-serial-probe.py /tmp/a19-ui-serial.sock \
  logs/ui-probe-client.log --timeout 240
```

`wait=on` ensures that early UART boot output is collected. A long-lived service
may outlast the collector deadline; query QMP status and inspect the transcript
before deciding whether to stop the experiment.

The v7 probe was stopped through QMP after collecting these service failures.
Its timeout wrapper had not returned, so no backboardd exit status or complete
UI_PROBE_END marker is claimed for this run.


## Managed backboardd and dependency probes

The `service-root.dmg` experiment is an APFS clone of `ui-root.dmg`. It uses the
original 24A437 backboardd launch plist with `KeepAlive=false` and `_PanicOnCrash`
removed so failures remain inspectable. `UserName=mobile`, `_Conclave`, and the
original MachServices are preserved. See the
[exact diagnostic plist](evidence/backboard-service-plist.json).

Redirecting mobile backboardd's stdout/stderr to `/dev/console` caused xpcproxy
to fail with permission denied and exit 78 (v8). Removing those probe-only
redirections allowed launchd to report the service running (v9). No HID
mach-register sandbox denial was observed in that run. This establishes a
better launch context, not a working display or a verified Conclave.

The independent Bash job was restored for v10. Its launchd inspection probe
returned two jobs, including backboardd; an IORegistry query found a registered
`IOSurfaceRoot` / `IOCoreSurfaceRoot`. No IOHIDSystem was returned by that query.
A registered IOSurface driver is not evidence of successfully creating or
rendering a surface.

### Guest service control

`launch-probe.c` is a small substitute for the unavailable launchctl executable.
It supports `list`, `submit PLIST`, and `lookup MACH_SERVICE` using the legacy
launch API and bootstrap lookup. It operates in the caller's bootstrap domain;
it does not implement domain switching or launchctl policy preprocessing.
A successful lookup establishes a reachable service port, not responsiveness
or successful initialization of the service behind it.

Build against the matching restore libraries and sign it:

```sh
xcrun clang -Wall -Wextra -Werror -target arm64-apple-ios27.0 \
  -isysroot /tmp/a19-restore -nostdlib research/launch-probe.c \
  /tmp/a19-restore/usr/lib/libSystem.B.dylib \
  /tmp/a19-restore/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation \
  -o /tmp/launch-probe
codesign -s - /tmp/launch-probe
```

As with cache-diagnostics, copy it into guest `/bin` and include the current
CDHash in the guest trust cache. The matching notifyd, cfprefsd,
MobileGestaltHelper, logd and runningboardd binaries and their source plists
were staged in the service image; only services explicitly installed or
submitted are enabled. The original staged plists live under guest `/launchjobs`.

V11 confirmed the `SubmitJob` calls returned errno 0, and the user preference
job appeared in GetJobs. However, notifyd also logged
`Non-system service tried to claim event stream com.apple.notifyd.matching`.
Thus submission success is not sufficient validation. The next image installs
notifyd and both cfprefsd jobs in the normal boot LaunchDaemons directory.
`service-probe.sh` checks their Mach ports and the backboard HID/render ports,
then lists jobs. The original direct-execution `ui-probe.sh` remains separate.

### Root-owned plist preparation without changing host privileges

The diagnostic image's original Bash plist was confirmed as uid/gid 0:0 on a
host mount with ownership enabled. An in-place write on the ownership-disabled
mount preserves that inode's ownership. It was rewritten with the backboardd
configuration and renamed to `com.apple.backboardd.plist`.

For additional jobs, disposable root-owned command inodes were renamed into
the LaunchDaemons directory, overwritten in place with the desired plist, and
chmodded to 0644. The donors were `/bin/yes` for Bash, `/bin/factor` for notifyd,
`/bin/base32` for system cfprefsd, and `/bin/shuf` for user cfprefsd. Those commands
are consequently absent in this disposable image. This is a preparation
workaround, not a proposed installation format; the baseline and original
probe images retain their tools. A maintained full-system image should be
prepared with proper ownership directly.

### Virtual-display lead

The exact QuartzCore cache has `CAWindowServerVirtualDisplay` and a
`ca_virtual_main_display` boot option. Disassembly of
`___CADeviceUseVirtualMainDisplay_block_invoke` at 0x1847400bc shows the option
is read only when `CADeviceHasInternalBuild` is true; otherwise the result is
false. This is a potential research path, not a verified software-rendering
configuration. Symbol aliases reported by the bulk ipsw disassembler can be
misleading, so individual call targets require verification before relying on
them. No QuartzCore code or shared-cache signatures were modified.


V12 verified the boot-managed configuration: launchd reported notifyd,
system cfprefsd and user cfprefsd running. Backboardd remained listed with a
PID, and lookups of `com.apple.iohideventsystem`, `com.apple.CARenderServer`,
`com.apple.system.notification_center`, `com.apple.cfprefsd.daemon.system`, and
`com.apple.cfprefsd.daemon` all returned success and a Mach port. The probe
completed its `UI_PROBE_END` marker. See
[managed-service excerpts](evidence/managed-services-excerpts.txt).

The next diagnostic, `display-probe.c`, loads guest QuartzCore and invokes
`+[CADisplay displays]` and `+[CADisplay mainDisplay]` through the Objective-C
runtime. These selectors were verified in the exact cache's symbol listing.
It reports checkpoints before potentially blocking calls. Build it like
cache-diagnostics against restore libSystem, sign it and add its CDHash to the
trust cache. This queries the actual display stack; it does not create a
synthetic display or render host-generated graphics.


V13 reached `DISPLAY_PROBE_BEGIN` and `CADISPLAY_QUERY_BEGIN`, proving the
QuartzCore load and runtime lookup succeeded. The `+[CADisplay displays]` call
had not returned on subsequent observations, while QMP reported the VM running
and SEP timeout messages continued. This does not establish a permanent hang
or its cause; it establishes that the display API did not complete within the
observed interval. That experiment was later stopped explicitly for the thread-sampling tests. Raw local transcripts are in `logs/ui-probe-v13-*.log`.


## Developer Mode and debugging (v14–v16)

A tool signed with `com.apple.private.cs.debugger` was killed before execution
because Developer Mode was off (v14). Removing that entitlement exposed the
same restriction on `com.apple.system-task-ports` (v15). These were guest AMFI
restrictions, not limitations of the host debugger or inability to inspect QEMU.

The exact AMFI binary has an existing `-restore` boot-argument path. Adding it
caused `AMFI: Enabling developer mode since we are restoring....`, but this log
alone was misleading: the guest sysctls still reported status=0, resolved=1,
and the entitled tool was still rejected. `cache-diagnostics devmode` reads
both sysctls without changing them.

### Verified temporary override

LLDB attached successfully through QEMU's loopback-only GDB server, recognized
the kernel's 0x20000000 slide, read CPU registers, and detached/resumed the VM.
The kernel Developer Mode getter reads a pointer at 0xfffffe0027ea1600. In this
exact boot, it points to TXM's state byte at 0xfffffe0017088db4. Reading that byte
returned 0; writing 1 through LLDB changed the actual TXM state read by the
kernel, not just the sysctl display.

After resuming, the guest reported `security.mac.amfi.developer_mode_status=1`.
The previously rejected task inspector executed, obtained task ports for both
the display client and backboardd (`task_for_pid` result 0), read their thread
states and stack candidates, and resumed each target successfully. See
[verification evidence](evidence/developer-mode-v16.txt).

This is an explicit, temporary debugger override of the disposable guest's
security state. It is lost on reboot and is **not** proof that the normal
Developer Mode enablement flow or persistent settings work. The normal device
workflow is documented by [Apple](https://developer.apple.com/documentation/xcode/enabling-developer-mode-on-a-device).
No host security setting was changed.

The addresses above apply only to the tested loader layout and firmware:

- TXM SHA-256: `e058931840ac000f6d050578fd7fc4ddb8c640125d92a713f61433be793fbf04`
- Diagnostic bootkc SHA-256: `77fc882042df1c309a6f47ebb893bcf73dd4af19e3f0a5198f195f71bf0a727d`

Debugger setup for that stopped/known experiment:

```sh
python3 research/qmp.py /tmp/a19-ui-v16-qmp.sock human-monitor-command \
  '{"command-line":"gdbserver tcp:127.0.0.1:63417"}'
xcrun lldb firmware/iphone-17-ui-probe/bootkc-cache-owner-probe
# LLDB:
# gdb-remote 127.0.0.1:63417
# memory read --format x --size 8 --count 1 0xfffffe0027ea1600
# memory read --size 1 --count 1 0xfffffe0017088db4
# After verifying the expected pointer and byte for this exact firmware:
# memory write --size 1 0xfffffe0017088db4 1
# process detach
```

The v16 guest was subsequently stopped for the no-SEP configuration experiment;
use the socket of the actual live experiment, not an old socket filename.

### Thread sampling and the display wait

`thread-probe.c` provides a bounded frame-pointer sampler. `display-probe.c`
links it with `THREAD_PROBE_LIBRARY` and samples its own blocked thread after
five seconds, without special entitlements. The standalone build attempts
`task_for_pid` using `thread-probe-entitlements.plist`; Developer Mode must be
active for these entitlements. The first standalone experiment suspended the
whole target. The current implementation suspends/resumes individual threads,
skips its own sampler thread, and resumes before printing to avoid a same-process
stdio-lock deadlock. Its self-sampling path was verified in v16.

Build using the installed Darwin SDK headers and the matching iOS libSystem:

```sh
xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib -DTHREAD_PROBE_LIBRARY \
  research/display-probe.c research/thread-probe.c \
  /tmp/a19-restore/usr/lib/libSystem.B.dylib -o /tmp/display-probe
codesign -s - /tmp/display-probe

xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib research/thread-probe.c \
  /tmp/a19-restore/usr/lib/libSystem.B.dylib -o /tmp/thread-probe
codesign -s - --entitlements research/thread-probe-entitlements.plist /tmp/thread-probe
```

Install and trust-cache these binaries as before. Saved return addresses are
masked to their low 39 bits as a PAC-stripping heuristic; this is not a general
unwinder. The helper reports the guest shared-cache slide for symbolication.
Subtract that slide before using `ipsw dyld a2f --in ADDRESSES --json`.
Bulk symbol aliases can be wrong (one Mach-message frame was labeled as an
unrelated Objective-C method), so interpret names alongside image ranges and
neighboring frames.

The display client stack goes through `+[CADisplay displays]`, `ensure_displays`,
`query_displays`, `__CASGetDisplays`, and Mach-message receive. Backboardd's main
thread is in `IOServiceOpen` through AppleKeyStore connection setup and
`+[BMDataProtection isClassCXUnlocked]`, called during Biome initialization.
These samples identify a key-store/data-protection dependency before display
initialization can be assessed. They do not prove that this is the only blocker.
[Client symbols](evidence/display-wait-symbols.json) and
[backboardd symbols](evidence/backboard-wait-symbols.json) retain the raw tool
output, including the alias limitation above.

### No-SEP configuration experiment

The stripped tree still advertises `sepfw-load-at-boot=1` although no working
SEP is emulated. `disable-sep-boot.py` changes only that existing four-byte
property to 0 in a new file, preserving all other tree bytes. `DTREE` can now
override the tree path in `run-boot-display.sh`. This is an experiment in
accurate hardware configuration, not SEP emulation. The first test is v17,
using `dtree-no-sep` and the same `-restore` boot arguments.


V17 did **not** resolve the issue: even with `sepfw-load-at-boot=0`, the kernel
logged `_sep_enabled = 1`, Developer Mode remained status=0/resolved=1, and the
display client's sample showed the same CASGetDisplays/Mach receive path.
The byte-level tree edit was verified, but changing that one property is not
sufficient to put this driver stack into a no-SEP mode.

The latest v17 guest was then given the same temporary TXM state override via
LLDB and left running for further inspection. Its QMP socket is
`/tmp/a19-ui-v17-qmp.sock`, UART socket `/tmp/a19-ui-v17-serial.sock`, and GDB
endpoint `127.0.0.1:63417` (loopback only). The next investigation is the
AppleKeyStore user-client open, now accessible to the guest task inspector.


### Direct AKS endpoint experiment (v18)

Disassembly identified a separate `aks-endpoint` boot argument. Setting it to
zero sets the driver field at offset `0x1f0`; the helper that checks SEP boot
readiness tests that field before its `sep-booted` / `sepfw-load-at-boot` paths.
[Relevant instructions](evidence/aks-endpoint-disassembly.txt) are from the exact
24A437 AppleSEPKeyStore image. This establishes a diagnostic control, not a
working replacement for SEP.

V18 kept the v17 configuration and added `aks-endpoint=0`. It booted to the
shell, but the display query still blocked. After the same verified temporary
Developer Mode override, `task_for_pid(4)` succeeded and backboardd was sampled.
All 24 main-thread saved return addresses match v16 after subtracting their
respective shared-cache slides (`0x8a74000` versus `0x1441c000`). It still waits
in the AppleKeyStore `IOServiceOpen` path during Biome initialization.
[Guest evidence](evidence/aks-endpoint-v18-sample.txt).

Correction to interpreting the earlier experiment: the `_sep_enabled = 1`
message uses a literal 1 in this build's logging call (`mov w10, #1` at
`0xfffffe00095bf3a4`). That message alone cannot tell whether these configuration
changes took effect. Neither tested change has unblocked the observed client.

V17 is stopped. The current v18 guest is running with QMP
`/tmp/a19-ui-v18-qmp.sock`, UART `/tmp/a19-ui-v18-serial.sock`, and loopback GDB
`127.0.0.1:63418`. Developer Mode was temporarily set to 1 and read back in the
guest. The next direction is to trace the kernel-side user-client open rather
than infer driver readiness from the startup log.


### Kernel-side user-client trace (v19)

A fresh boot with QEMU `-S -gdb tcp:127.0.0.1:63419` allowed hardware
breakpoints before managed backboardd starts. `ipsw kernel cpp --methods`
recovered the stripped AppleKeyStore / AppleKeyStoreUserClient / IOWorkLoop
vtables. The user-client startup entry is unslid `0xfffffe00095ca83c`.

The trace passed base startup and the entitlement-check sequence. It did not
reach the instruction after the work-loop call at `0xfffffe00095cab0c`.
Live object reads confirm the newly allocated command gate and the actual
IOWorkLoop vtable. The resolved call matches `IOWorkLoop::addEventSource`,
which delegates to its control gate; compare [Apple's implementation](https://github.com/apple-oss-distributions/xnu/blob/main/iokit/Kernel/IOWorkLoop.cpp).
This narrows the wait to event-source attachment or its callees, rather than a
completed user-client open followed by a user-space key operation. Identifying
the gate owner and its blocked kernel stack is the next step; SEP causality
is still unproven.

[Breakpoint observations](evidence/aks-workloop-v19.md) and
[recovered method slots](evidence/aks-workloop-methods.json) preserve addresses
and interpretation limits. A second unmanaged backboardd test in v18 exited
because its HID endpoint already belonged to the managed job; it did not
reach this breakpoint. Use the managed startup for reproducing the trace.

V18 is stopped. V19 is now running at `/tmp/a19-ui-v19-qmp.sock`, UART
`/tmp/a19-ui-v19-serial.sock`, GDB `127.0.0.1:63419`. All trace breakpoints were
removed before detaching; the temporary TXM Developer Mode override was applied
and read back. The automatic UI probe still did not complete its CADisplay
query. There is no graphical-startup success claim.


### Gate owner found; endpoint-order patch removes the wait (v20)

The v19 work-loop gate has owner `0xfffffe25c9fed8a0` and recursion count 2.
Live disassembly verifies gateLock at workLoop +0x10, owner at lock +0x18,
and count at +0x20. Its saved-stack candidate chain includes AKS returns
`0xfffffe00095f0458`, `0xfffffe00095c5dfc`, and `0xfffffe00095c5f70`.
See [reads](evidence/aks-gate-owner-read.txt) and
[candidate frames](evidence/aks-gate-owner-frames.json); the latter uses a
bounded pointer-chain/PAC heuristic, not a fully validated kernel unwinder.

The matching disassembly explains why `aks-endpoint=0` failed: the helper calls
the AppleSEPManager lookup at `0xfffffe00095c5df8` **before** checking the
endpoint-disable field. That lookup constructs the `AppleSEPManager` service
match and waits with timeout argument -1 at `0xfffffe00095f0454`. The observed
owner stack sits in this path while holding the work-loop gate.

`patch-aks-endpoint-order.py` reorders four instructions so the existing disable
check runs before the lookup. It requires the exact diagnostic bootkc hash,
verifies old bytes, and writes a separate output. With the flag unset, the
original lookup still runs. This is an experimental firmware-specific control-
flow patch, not SEP emulation or a fix for unavailable keys.

```sh
python3 research/patch-aks-endpoint-order.py   firmware/iphone-17-ui-probe/bootkc-cache-owner-probe   firmware/iphone-17-ui-probe/bootkc-aks-endpoint-order
# Run the previous configuration with this BOOTKC and aks-endpoint=0.
```

V20 boots successfully. Backboardd PID 4 now reaches AKS external selectors
17 and 7, which return `e00002d8` for unavailable endpoint operations. It no
longer stays in the old user-client-open wait. It subsequently exits with
SIGABRT after about six seconds; two on-demand restarts also abort after about
two seconds. The display query does not report a display count. This is a
verified removal of the earlier wait, **not** graphical startup success. The
abort's cause has not yet been identified; failed service lookups alone are
not sufficient to attribute it.

[Patch hashes](evidence/aks-endpoint-order-patch.json) and
[boot excerpts](evidence/aks-order-v20-excerpts.txt) record the experiment.
V19 is stopped. Current v20 sockets are `/tmp/a19-ui-v20-qmp.sock` and
`/tmp/a19-ui-v20-serial.sock`, with GDB at `127.0.0.1:63420`. Temporary Developer
Mode was enabled after boot. The next direction is capturing backboardd's
abort stack, now that it can progress beyond key-store initialization.


### Backboardd's next abort: missing system-container path (v20/v21)

Hardware breakpoints on the exact shared-cache `_abort` and
`_objc_exception_throw` captured an `NSInvalidArgumentException` with reason
`*** -[NSURL initFileURLWithPath:]: nil string parameter`. The call chain includes
`-[BSSystemContainerForCurrentProcessPathProvider libraryPath]` and `cachesPath`.
This identifies a missing system-container path, not a graphics-driver crash.
[Exception observations](evidence/backboard-abort-v20.md) and
[symbolication](evidence/backboard-abort-v20-symbols.json) retain the evidence.
Cache addresses require the v20 slide `0x1a030000`; they are not universal.

To test the missing service, v20 was stopped and `service-root.dmg` mounted
with `hdiutil attach ... -mountpoint /tmp/a19-service-root -owners off -nobrowse`.
The original 24A437 `/usr/libexec/containermanagerd_system` was copied from the
mounted system image. Its unmodified
`System/Library/LaunchDaemons/com.apple.containermanagerd.system.plist` was
written into the root-owned `/bin/b2sum` donor inode, renamed into
LaunchDaemons, and chmodded 0644. As with the earlier donor workaround,
`b2sum` is consequently absent only in this disposable image. The existing
merged system trust cache already covers the original signed helper. The image
was detached before booting with the same AKS-patched kernel and arguments.

V21 proves that launchd starts the helper in the system domain on demand.
It exits with status 0 after about 0.25–0.74 seconds and does not resolve the
failure: backboardd still exits with SIGABRT and CADisplay reports no count.
A logged activation of `com.apple.containermanagerd` fails with Operation not
permitted. Neither that log nor the exit code alone establishes why the helper
quits. The normal container directories are absent from this restore image;
its storage/layout requirements remain to be investigated rather than assumed
fixed by installing a binary. [V21 log excerpts](evidence/container-service-v21.txt).

V20 is stopped and all its debugger breakpoints were removed. Current v21 is
running with QMP `/tmp/a19-ui-v21-qmp.sock` and UART
`/tmp/a19-ui-v21-serial.sock`. No GDB server or temporary Developer Mode override
has yet been enabled for v21. Its shared-cache slide is `0x4e8c000`. The next
step is tracing ContainerManagerCommon's helper startup/exit and preparing the
required writable container environment. Graphical startup remains unverified.


### Container service permanent errors and storage checks (v21/v22)

V21's `_exit(0)` originated from
`____containermanagerd_reply_with_error_block_invoke`; it is an error-response
exit, not evidence of a working daemon. A breakpoint on the permanent-error
listener captured an MCMError whose type was 102. The exact firmware's error
string table maps this to `USER_HOME_DIRECTORY_MISSING`.
[Debugger observations](evidence/container-permanent-error-v21.md).

Independent [mount/write checks](evidence/v21-storage.txt) confirmed that the
root APFS ramdisk is read-only, including `/private/var/tmp`. An attempted
256 MiB tmpfs mount on that directory was denied by System Policy's file-mount
check and failed with Operation not permitted; no writable filesystem was
created. [Tmpfs evidence](evidence/v21-tmpfs.txt).

For v22, `/private/var/root` and `/private/var/mobile` were created with mode
0755 on the stopped image's ownership-disabled host mount. Ownership and
writability are not solved by this experiment. The permanent error changed to
149, `INVALID_CONFIG_FILE`, proving that the helper got beyond the previous
home-path check. Backboardd still aborts. [V22 evidence](evidence/container-error-v22.md).

The original full system image contains `ContainerManagerCommon.framework`
resources including `Containers.plist`, `Platform.plist`, `SingleDaemon.plist`,
`User.plist`, and multiple Container.Class/Container.User plists. These were not
staged when the dyld cache was installed. Inspecting/staging the matching
configuration is the next step, with writable /var still an independent
requirement.

V21 is stopped. Current v22 is running with QMP `/tmp/a19-ui-v22-qmp.sock`, UART
`/tmp/a19-ui-v22-serial.sock`, and loopback GDB `127.0.0.1:63422`. All breakpoints
were removed. Developer Mode has not been overridden in v22. Graphical startup
is not complete.


### Matching ContainerManager resources staged (v23)

The complete `System/Library/PrivateFrameworks/ContainerManagerCommon.framework`
resource directory was copied from the exact system image into the stopped
service ramdisk. All 28 plist files were byte-compared against the source;
[resource hashes](evidence/container-config-resources.json) record the inputs.
No kernel, launch configuration, or boot argument changed for this test.

The permanent error changed from `INVALID_CONFIG_FILE` (149) to `DURING_STARTUP`
(91). [Live error observations](evidence/container-error-v23.md). Configuration
loading is no longer the observed failure, but the new error is generic: its
specific cause remains unproven. Backboardd still aborts and the display query
does not report a display count.

The read-only /var and denied tmpfs mount remain unresolved. An entitlement
comparison found that the restore image's mount_tmpfs has no listed
entitlements, while restored_external has `com.apple.private.security.no-sandbox`
and `com.apple.private.security.disk-device-access`. A trusted diagnostic mount
helper with these restore-service entitlements is a concrete next experiment;
it has not yet been installed or tested. It should first prove an isolated
writable mount before any broader /var setup.

V22 is stopped. V23 is running at QMP `/tmp/a19-ui-v23-qmp.sock`, UART
`/tmp/a19-ui-v23-serial.sock`, and GDB `127.0.0.1:63423`. All hardware breakpoints
were removed. Developer Mode has not been overridden in v23. Graphical startup
remains incomplete.


### Writable tmpfs verified at restore mount points (v24)

The location, not additional entitlements, is the useful distinction. The
original restore mount_tmpfs succeeds at `/mnt2`; actual file creation and
readback succeed there. A diagnostic signed copy also succeeds at `/mnt1`, but
still fails at `/private/var/tmp`. The root APFS remains read-only.

Experiments, in order:

1. Copy the original restore tmpfs helper to `/bin/mount-tmpfs-probe`, sign with
   `mount-probe-entitlements.plist` (no-sandbox and disk-device-access), and add
   its CDHash to the merged v1 trust cache. [Build record](evidence/mount-probe-build.json).
2. That copy is still denied file-mount at `/private/var/tmp`.
   [Denied attempt](evidence/v24-mount-test.txt).
3. The same copy mounts 256 MiB tmpfs at the pre-existing `/mnt1`.
   [Mount-point comparison](evidence/v24-mountpoint-test.txt).
4. The **original unmodified** `/sbin/mount_tmpfs -s 67108864 /mnt2` succeeds.
   Writing and reading test files on both mounts succeeds.
   [Write/readback proof](evidence/v24-writable-proof.txt).

No guest kernel policy patch or Developer Mode override was needed. The
entitlement experiment is retained as evidence but is unnecessary for future
mounts at the tested restore locations. The diagnostic binary remains in this
disposable image; the original helper and baseline image were preserved.

`serial-command.py` captures a short command on an already idle guest shell,
using paced UART writes and a unique full-line completion marker. It was used
for all three v24 experiments. It must have exclusive UART access; a timeout
leaves the guest running and does not imply the command stopped. The marker
proves shell completion only: inspect the command's reported result separately.

This enables the next experiment: stage a disposable `/var` mapping to a tmpfs
at an allowed restore mount point, seed required directories with guest-side
ownership, and initialize it before graphical service requests. **That mapping
has not yet been implemented.** Tmpfs is volatile and does not solve persistent
data storage or missing SEP/data-protection behavior.

V23 is stopped. V24 is running with QMP `/tmp/a19-ui-v24-qmp.sock` and UART
`/tmp/a19-ui-v24-serial.sock`; `/mnt1` and `/mnt2` are currently mounted writable.
No GDB server or Developer Mode override has been enabled in v24. `/var` is
still read-only and graphical startup remains incomplete.


### Early writable /var experiment (v25)

On the stopped image, the existing `private/var` directory was renamed to
`/var-seed`, and `/private/var` became a symlink to `/mnt1`. The root Bash
launch job now runs `/bin/bash /bin/init-writable-var`, installed from
[`init-writable-var.sh`](init-writable-var.sh). That script mounts 512 MiB tmpfs,
copies the seed, creates runtime directories, sets root/mobile ownership,
verifies a write/readback, and finally execs an interactive Bash. Backboardd's
RunAtLoad was set false; its existing Mach services start it on demand after
the readiness marker and shell prompt. Other launchd activity can occur before
the script: an early mobile/tmp fixup still reports read-only filesystem.

The script completed. Guest numeric ownership is root 0:0 (0700), mobile
501:501 (0755), and tmp 0:0 (1777). A second post-startup write/readback through
`/var/tmp` succeeds. [Storage proof](evidence/v25-var-proof.txt).

The container helper nevertheless returns DURING_STARTUP. A breakpoint at
`MCMLibraryRepair::createPathsIfNecessaryWithError:`'s caller confirms a false
return and NSPOSIXErrorDomain code 1 (EPERM). The sandbox log names the blocked
resolved path `/mnt1/root/Library/MobileContainerManager`.
[Debugger findings](evidence/container-path-creation-v25.md) and
[startup excerpts](evidence/v25-var-startup-excerpts.txt).

This identifies the limitation of the symlink layout: shell writes work, but
service sandbox rules see `/mnt1/...`, not an allowed canonical data path.
A policy-compatible data-volume layout or a narrowly scoped diagnostic service
policy adjustment is still required. Do not treat the shell write proof as
proof that full iOS services can use this /var mapping. Persistent storage,
APFS-specific metadata, and SEP/data protection also remain unresolved.

V24 is stopped. V25 is running at `/tmp/a19-ui-v25-qmp.sock`, UART
`/tmp/a19-ui-v25-serial.sock`, with loopback GDB `127.0.0.1:63425`.
All hardware breakpoints were removed and LLDB detached; no Developer Mode
override was applied. /var remains backed by volatile tmpfs, while backboardd
still aborts and no graphical display has been established.

## Container path exception and migration crash (v26)

Preserving the helper's original entitlements and adding an absolute-path
read-write exception for `/mnt1/` fixes its observed directory-creation
denial. LLDB confirmed success with a nil error, and the expected directories
were created. The helper now progresses into build-upgrade migration and
crashes with SIGSEGV. Breakpoint tracing narrows the failure to the final
portion of that migration method; see the [evidence and next breakpoints](evidence/container-migration-v26.md).

This remains a diagnostic helper build, not a completed container or graphics
implementation. The v26 guest is running with all breakpoints removed and
LLDB detached; Developer Mode was not overridden.

## ICU resources resolve container migration startup crash (v27–v28)

Tracing refined the v26 crash interval to
`-[MCMMigrationStatus _iso8601DateFormatter]` during migration-completion
bookkeeping. The guest lacked `/usr/share/icu`. Adding the matching 24A437
`icudt78l.dat` and `icutzformat.txt`, without executable changes, got the
helper past startup. See [debugger evidence](evidence/migration-date-formatter-v27.md)
and [resource hashes](evidence/icu-resources-v28.json).

In v28, `containermanagerd_system` PID 31 remains alive on repeated checks
and writes `mcm_migration_status.plist`. The [decoded status](evidence/migration-status-v28.json)
records `ExcludePSCFromBackup` for build 24A437 at `1970-01-01T00:00:24Z`.
The 1970 timestamp reflects this guest's clock, not host time. It also creates
a system container with metadata, Documents, Library/Caches,
Library/Preferences, and tmp. These are stronger evidence than an absence
of crashes alone.

The next observed denial is:

```text
Sandbox: containermanagerd_system(31) deny(1) file-issue-extension target:/mnt1/containers/Data/System/F44F7D22-E8DE-4FD4-95EF-F861F7B6755C extension-class:com.apple.sandbox.system-container
```

The scoped read/write entitlement fixes directory creation but does not
permit issuing this extension at the resolved tmpfs path. Container child
directories are currently mode 000, owned by nobody; successful client
access has not been demonstrated. Backboardd PID 26 still exits with SIGABRT.
Do not treat the resource fix as completed container access or graphical boot.

The old v26 and v27 guests eventually hit SPTM/nested kernel panics after
repeated failures; their relationship to the helper fault remains unknown.
Both were stopped after confirmed panic, not an observation timeout.
The fresh v28 guest is running, with no debugger attached or Developer Mode
override. QMP: `/tmp/a19-ui-v28-qmp.sock`; UART: `/tmp/a19-ui-v28-serial.sock`.
Only resource hashes, decoded diagnostic output, and notes are committed;
Apple's ICU files remain local firmware assets.

## Native var mount and BackBoard configuration (v29–v30)

The original tmpfs helper **can mount directly at `/private/var`**. Earlier
denials were for `/private/var/tmp`; they did not establish a denial at the
parent mount point. Making `/private/var` a real directory and mounting there
avoids the resolved `/mnt1` sandbox-path mismatch. `init-writable-var.sh` now
uses this layout and drops to an interactive shell if mounting fails.

The v29 run produced no container-extension denial and reached a different
exception: `NSInternalInconsistencyException` for the missing
`/System/Library/BackBoard/EventProcessorConfiguration.plist`. Before abort,
thread samples showed `CAWindowServer _detectDisplays`, render-server startup,
and IOMobileFramebuffer display-list population. [Evidence and reproduction](evidence/native-var-v29.md).
This establishes execution progress, not a detected display or rendered frame.

V30 restores `containermanagerd_system` byte-for-byte from 24A437 and adds the
matching 1202-byte BackBoard configuration. [Input hashes](evidence/native-var-v30-inputs.json).
The previous scoped `/mnt1/` entitlement is no longer present in the installed
helper. Historical diagnostic CDHashes remain in the merged trust cache; no
new trust entry was needed. The attempted no-sandbox experiment was rejected
by automatic approval review and never executed; the native-path alternative
required no sandbox bypass.

V29 was stopped after its debugger detached and all breakpoints were removed.
V30 uses QMP `/tmp/a19-ui-v30-qmp.sock` and UART
`/tmp/a19-ui-v30-serial.sock`. Developer Mode has not been overridden in v30.

The follow-up probe waited 45 guest seconds after the initial UI probe, then
reported backboardd PID 26 with `LAST_EXIT=0`, helper PID 31 alive, and tmpfs
still mounted at `/private/var`. [Survival and mount evidence](evidence/native-var-v30-survival.txt).
This passes the previous approximately 38-second abort window. No SIGABRT,
SIGSEGV, or sandbox denial appeared in the captured v30 log at checkpoint.
The CADisplay query still has no completed display count, so further service
and display-driver investigation is required. QMP confirms the guest running.

## Display enumeration succeeds, but only wireless placeholders (v30–v31)

A later v30 query completes with `CADISPLAY_COUNT=5`,
`CADISPLAY_MAIN_PRESENT=0`, and exit status 0. [Query evidence](evidence/display-query-v30.txt).
The earlier query issued during startup did not provide a usable result; a
fresh query after startup is necessary when reproducing this observation.

With temporary Developer Mode enabled, live thread inspection found backboardd
PID 26 still alive, its main thread in CFRunLoop, and QuartzCore render-server
and IOMFBServer threads running. [Raw sample](evidence/backboard-idle-sample-v30.txt)
and [symbolication](evidence/backboard-idle-threads-v30.json). These sampled
wait states alone do not prove rendering. LLDB detached before v30 stopped.

`display-probe.c` now reports each display's ID, name, external/support flags,
and current mode, using selectors verified in the exact 24A437 cache. It limits
enumeration to 32 objects and reports truncation. The rebuilt diagnostic uses
no entitlements; [build hash and trust entry](evidence/display-probe-v31-build.json).
It compiled with `-Wall -Wextra -Werror` and ran successfully in v31.

The [v31 inventory](evidence/display-inventory-v31.txt) is:

| IDs | Names | External | Supported flag | Current mode |
| --- | --- | --- | --- | --- |
| 1–5 | Wireless, Wireless-1 through Wireless-4 | 1 | 1 | 0 × 0, undefined range |

No main display is present. The supported flag does not make these zero-sized
wireless objects usable outputs. This is successful server/client enumeration,
not five connected monitors or graphical boot.

### Next virtual-display experiment

The exact cache contains `-[CAWindowServerVirtualDisplay initWithOptions:]` at
`0x1847678e0`. Inspection of its argument keys identifies
`kCAVirtualDisplayWidth`, `kCAVirtualDisplayHeight`, and
`kCAVirtualDisplayUpdateRate`. Width and height are required according to its
embedded diagnostic string. The constructor also has pixel-format and physical
size options. A bounded standalone virtual-display construction probe was subsequently
implemented and tested in v32/v33 (below). Neither framebuffer allocation,
CPU rendering, attachment to backboardd, nor use as the main display is
established by these symbols alone. The earlier boot-option path remains
gated by the internal-build check; no QuartzCore code or feature flags were
modified.

V31 is running at QMP `/tmp/a19-ui-v31-qmp.sock` and UART
`/tmp/a19-ui-v31-serial.sock`. No debugger or Developer Mode override was used
in v31. The original sandboxed container helper and native `/private/var`
mount from v30 remain in use. SpringBoard, usable display modes, rendering,
and interactive input are still unfinished.

## Virtual display construction succeeds (v32–v33)

`virtual-display-probe.c` dynamically loads the matching guest QuartzCore and
constructs `CAWindowServerVirtualDisplay` with width 1024, height 768, and
update rate 60. It has no entitlements and does not register a render service
or attach its display to backboardd. A 20-second process-only alarm bounds the
experiment; its existing thread sampler can report a stall after five seconds.

The [v32 construction result](evidence/virtual-display-construction-v32.txt)
returned a non-null object named `Virtual-0`, ID 1, with bounds
`0,0,1024,768`, and process exit status 0. These are local object properties;
ID 1 is not evidence that backboardd's Wireless display was replaced.

The expanded v33 probe prints runtime method encodings and accepts
`--render-empty`. It verifies that `renderForTime:` takes a double and returns
void before invoking it. [Observed result](evidence/virtual-display-empty-render-v33.txt):

```text
VIRTUAL_METHOD=renderForTime: TYPE=v24@0:8d16
VIRTUAL_METHOD=acquireFrozenSurface TYPE=^{__IOSurface=}16@0:8
VIRTUAL_METHOD=beginExternalUpdate:usingSoftwareRenderer: TYPE=v28@0:8^v16B24
VIRTUAL_RENDER_EMPTY_RETURNED
VIRTUAL_FROZEN_SURFACE_PRESENT=0
VIRTUAL_EXIT=0
```

This proves object construction and return from an empty render invocation.
It does **not** prove framebuffer allocation, generated pixels, main-display
registration, or GUI rendering. The probe does not enable the display, submit
a layer context, or create an external render update; any of these may matter
to whether a surface is produced.

Static inspection finds that `VirtualServer::renderer()` at `0x184629cc4`
calls `CAMetalContextCreate` (`0x1847c341c`) and
`CA::OGL::new_metal_context` (`0x18458ec84`). Thus virtual-display construction
alone does not establish GPU independence. The runtime external-update method
exposes a software-renderer boolean; exported `CARenderUpdateBegin`,
`CARenderUpdateBegin2`, and `CARenderUpdateFinish` are concrete next leads for
a CPU rendering experiment, but their calling contracts still need verification.
No private update structure or surface pointer was guessed or dereferenced.

Build and run:

```sh
xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib -DTHREAD_PROBE_LIBRARY \
  research/virtual-display-probe.c research/thread-probe.c \
  /tmp/a19-restore/usr/lib/libSystem.B.dylib -o /tmp/a19-virtual-display-probe
codesign -s - /tmp/a19-virtual-display-probe
# Stage in the stopped disposable guest and merge its CDHash into the trust cache.
# In the guest:
/bin/virtual-display-probe --render-empty
echo VIRTUAL_EXIT=$?
```

The probe compiled with warnings treated as errors; construction and empty
render modes were exercised in the guest. [v32 build](evidence/virtual-display-probe-build.json)
and [v33 build](evidence/virtual-display-render-probe-build.json) preserve
binary hashes and trust-cache entry counts.

V33 remains running with QMP `/tmp/a19-ui-v33-qmp.sock`, UART
`/tmp/a19-ui-v33-serial.sock`, no attached debugger, and no Developer Mode
override. V31 and v32 were intentionally stopped for image updates.
The original sandboxed container helper, native var mount, ICU resources, and
BackBoard configuration remain in use. Full interactive graphical boot is
still unverified and unfinished.

A [post-experiment query](evidence/virtual-display-isolation-v33.txt) still reports
the original five zero-sized Wireless displays, no main display, and
backboardd PID 26 with `LAST_EXIT=0`. The standalone virtual object was not
installed as the system display.

## External software update and direct CPU backend (v34)

The [v34 probe](evidence/software-update-v34.txt) created an opaque render
update, added a 1024x768 dirty rectangle, and called the virtual display's
external-update selectors with `usingSoftwareRenderer:YES`. Begin returned;
finish returned false; `acquireFrozenSurface` remained null. The diagnostic
exited 0 (successful observation, not successful rendering).
The update ABI is corroborated by
[WebKit's QuartzCoreSPI.h](https://github.com/WebKit/WebKit/blob/283c780414b7c24e4a911c823af665f53a15e8f7/Source/WebCore/PAL/pal/spi/cocoa/QuartzCoreSPI.h)
and the exact firmware's constructor. No layer context was submitted.

Further static inspection explains why the boolean alone is not enough:
`beginExternalUpdate:usingSoftwareRenderer:` dispatches the flag through server
vtable slot 0x60. VirtualServer's slot resolves to `0x18462a324`, a bare return.
Its regular renderer instead calls Metal context creation. This is a property
of this implementation, not a claim that all virtual displays require a GPU.

A separate software backend is available in this binary:

- `CARenderOGLNew` (`0x1846a5b88`) forwards a callback table to
  `CARenderOGLNew_` (`0x1846a5ab8`).
- `kCARenderSoftwareCallbacks` (`0x1e9002ca8`) begins with constructor
  `0x1846a667c`, which ignores the incoming arguments, allocates 0x52b0 bytes,
  runs the base Context constructor, and installs SWContext's vtable.
- `CARenderSoftwareSetDestination` (`0x1846a6654`) dispatches to slot 0x4d0.
  SWContext's entry at `0x1e9004670` is `0x1846c91d4`, whose symbol demangles to
  `CA::OGL::SWContext::set_destination(void*, long, unsigned long, void*, long,
  int, int, int, int)`. It stores buffer pointers, strides, bits per pixel,
  and origin/size; it returns 1.
- `CARenderOGLRender` (`0x1846a5fd8`) accepts renderer and update pointers;
  `CARenderOGLFinish` is available separately.

These are unslid addresses specific to 24A437. The new
[software-render-probe.c](software-render-probe.c) uses exported entry points
and a 64x64 buffer with sentinel bytes around it. It is an empty-update smoke
test; actual layer composition and integration into backboardd remain required.

### Direct software renderer result (v35)

[Runtime evidence](evidence/software-render-v35.txt) confirms exported symbols
resolve and the direct CPU backend can be constructed in this guest:

```text
SW_RENDERER_PRESENT=1
SW_DESTINATION_RESULT=1
SW_UPDATE_PRESENT=1
SW_RENDER_EMPTY_RETURNED
SW_FINISH_RETURNED
SW_CHANGED_BYTES=0 GUARD_CHANGED_BYTES=0
SOFTWARE_EXIT=0
```

The destination contains 16,384 sentinel bytes, with 4,096 guard bytes on each
side. No destination or guard bytes changed. This proves successful execution
of the empty-update path, **not** that anything was drawn. It does not test
textures, fonts, compositing correctness, or performance. A local `CAContext`
with a colored `CALayer`, committed and passed through `CARenderUpdateAddContext`,
is the next concrete test. That exported function exists at `0x18469ee8c`;
WebKit's header supplies its opaque context/update contract.

Build command:

```sh
xcrun clang -Wall -Wextra -Werror -Wno-incompatible-sysroot \
  -target arm64-apple-ios27.0 -nostdlib research/software-render-probe.c \
  /tmp/a19-restore/usr/lib/libSystem.B.dylib -o /tmp/a19-software-render-probe-v35
codesign -s - /tmp/a19-software-render-probe-v35
# Stage in stopped guest, add CDHash to ramdisk.tc, then boot.
/bin/software-render-probe
```

[Build identity](evidence/software-render-probe-build.json) records the signed
binary hash. No entitlement or Developer Mode override was needed. V34 was
intentionally stopped for installation; current v35 uses QMP
`/tmp/a19-ui-v35-qmp.sock` and UART `/tmp/a19-ui-v35-serial.sock`.
A usable main display, actual UI pixels, and interactive input remain unfinished.
