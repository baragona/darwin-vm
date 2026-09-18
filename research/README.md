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
- The baseline exposes five zero-sized wireless displays. Guest debugger
  configuration now adds a 416 × 496 virtual LCD selected by CADisplay as
  the main display (v42); presentation and interaction remain unverified.
- Direct CPU composition of a local opaque red CALayer is verified: all 4,096
  pixels in a 64x64 output buffer match the expected color (v36).
- Alpha blending, layer movement/color changes, and repaint of the old position
  pass complete pixel comparisons across two frames (v37).
- IOSurface allocation/mapping works with a scoped driver-client entitlement;
  remote-context commit/flush and server rendering succeed after the initial timeout
  (v39), including with Developer Mode restored to off.
- Adding the missing public passwd database fixes SpringBoard's user-directory
  crash; its next verified assertion requires a non-null main display (v42).
- With the virtual LCD selected, SpringBoard passes its missing-display
  assertion and next rejects an inert process handle with pid -1 (v42).
- RunningBoard and matching CoreEmoji resources move SpringBoard into
  applicationDidFinishLaunching; it now fails on missing _systemAppInfo
  (v44). LaunchServices components are staged for the next test.
- The original per-user container agent gets LaunchServices into its server
  run loop (v46). SpringBoard still lacks its installed application record.
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

## First verified Core Animation pixels (v36)

`software-render-probe --layer` successfully rendered an opaque red CALayer
through the iOS software backend, without a GPU attachment, debugger override,
or extra entitlements. This advances the earlier empty-update result to actual
layer content. The probe creates a local CAContext, attaches a 64x64 CALayer
with a DeviceRGB red background, disables implicit actions, commits and flushes
the transaction, obtains the opaque renderContext, and adds it to the update
with CARenderUpdateAddContext. It uses CACurrentMediaTime for the update time.

[Guest transcript](evidence/software-layer-v36.txt):

```text
SW_LOCAL_CONTEXT_PRESENT=1
SW_LAYER_COMMIT_RETURNED
SW_RENDER_CONTEXT_PRESENT=1
SW_ADD_CONTEXT_RETURNED
SW_RENDER_LAYER_RETURNED
SW_CHANGED_BYTES=16384 GUARD_CHANGED_BYTES=0
SW_FIRST_PIXEL=0000ffff CENTER=0000ffff
SW_OUTPUT_BYTES=16384 CLOSE_RESULT=0
SOFTWARE_EXIT=0
```

The output was transferred from `/private/var/tmp/software-layer.raw` via
`/bin/base64` on the guest UART. Host verification decoded exactly 16,384 bytes
and compared the **entire buffer** against `bytes([0,0,255,255]) * 4096`.
All 4,096 pixels matched opaque red interpreted as BGRA8; both 4,096-byte guard
regions remained unchanged. The raw output's SHA-256 is
`c34fb4331b2d031d7c644860b54a678424c66ef12352fc165a91dc09840d98fd`.

Artifacts:

- [Raw guest output](evidence/software-layer-v36.bgra)
- [PNG conversion of that output](evidence/software-layer-v36.png)
- [Exact pixel verification](evidence/software-layer-v36-verification.json)
- [Signed probe identity](evidence/software-layer-probe-build.json)

This is a single local solid-color layer, not SpringBoard or the system's main
display. It does not yet establish multi-layer composition, alpha blending,
frame-to-frame updates, textures, text, presentation speed, or touch input.
Those can now be tested against actual output instead of inferred from object
construction. Connecting the software backend to the system render server and
a usable display remains necessary for the interactive emulator goal.

Build uses the v35 command above with the updated source. Run `/bin/software-render-probe
--layer` in the guest; the no-argument empty-update mode remains available.
V35 was intentionally stopped for installation. V36 remains running on QMP
`/tmp/a19-ui-v36-qmp.sock` and UART `/tmp/a19-ui-v36-serial.sock`.

## Verified alpha composition and frame updates (v37)

The `--scene` mode reuses one local CAContext, CPU renderer, and output buffer
for two frames. Frame 0 places a 24x16 blue layer with alpha 0.5 at layer
coordinates (8,12) over the opaque red root. Frame 1 moves it to (32,28) and
changes it to opaque green, with implicit animations disabled. Each update
marks the full 64x64 destination dirty; this does not test minimal dirty-region
tracking. The second frame deliberately retains the previous destination bytes
before rendering, so old content must actually be repainted.

[Guest transcript](evidence/software-scene-v37.txt) reports both frames completed,
16,384 output bytes each, and no changed guard bytes. The host retrieved both
buffers via base64 and [verified every pixel](evidence/software-scene-v37-verification.json):

| Frame | Rectangle BGRA | Rectangle pixels | Red background pixels | Raw buffer rectangle |
| --- | --- | --- | --- | --- |
| 0 | `80007fff` | 384 | 3,712 | (8,36), 24x16 |
| 1 | `00ff00ff` | 384 | 3,712 | (32,20), 24x16 |

All 4,096 pixels matched in each frame, including red in the old rectangle's
position after movement. The default software destination reverses the layer
Y direction: raw row origin is `64 - layer_y - height`. This agrees with the
destination setter's negative-stride path inspected in v35. The alpha result
is blue=128, red=127, alpha=255 for this DeviceRGB scene.

Actual guest outputs: [frame 0](evidence/software-scene-v37-0.png),
[frame 1](evidence/software-scene-v37-1.png). The adjacent `.bgra` files retain
all original output bytes. [Build identity](evidence/software-scene-probe-build.json)
records the tested signed executable. The probe compiled with warnings treated
as errors and exited 0; pixel correctness was separately verified on the host.

Reproduction after building/staging the updated probe:

```sh
# Guest:
/bin/software-render-probe --scene
echo FRAME0_BEGIN
/bin/base64 /private/var/tmp/software-scene-0.raw
echo FRAME0_END
echo FRAME1_BEGIN
/bin/base64 /private/var/tmp/software-scene-1.raw
echo FRAME1_END
# Host, using the captured UART transcript:
python3 research/verify-software-scene.py logs/v37-scene-pixels.txt research/evidence/software-scene-v37
```

This establishes a functioning local software compositor for the tested
solid-color layers. It does not yet connect remote application contexts or
backboardd's display to this renderer. Fonts, textures, performance, a usable
main display, SpringBoard, and interactive input remain unverified. Static
inspection identifies `-[CAWindowServer addDisplay:]` at `0x18475d698`, but its
existence is not proof that a standalone process can register the system's
main display; integrating at the render-server level remains the next gap.

V36 was intentionally stopped for installation. Current v37 remains running
with QMP `/tmp/a19-ui-v37-qmp.sock` and UART `/tmp/a19-ui-v37-serial.sock`.

## Remote render-server path: IOSurface and context submission (v38–v39)

A new [remote-layer-probe.c](remote-layer-probe.c) attempts to allocate a 64x64
IOSurface, create a remote CAContext, submit its own red CALayer, and render
only that layer into the surface. This tests the application-to-render-server
path rather than another local context. A process-only 25-second alarm bounds
the experiment. The exported `CARenderServerRenderLayer` wrapper at
`0x1846dbdb0` forwards `(server_port, context_id, CALayer*, IOSurfaceRef, x, y)`
and returns a boolean. The underlying `0x1846db1e8` converts the CALayer's
internal object pointer to a render identifier and sends a Mach request.
Its default display string is `defaultDisplay` (CFString at `0x1e901aa60`,
text at `0x184818d85`), not a hardcoded `LCD` name from old examples.

[V38 evidence](evidence/remote-layer-v38.txt) stops before remote context creation:

```text
System Policy: remote-layer-probe(37) deny(1) iokit-open-user-client IOSurfaceRootUserClient
REMOTE_SURFACE_PRESENT=0
REMOTE_EXIT=1
```

The original 24A437 SpringBoard signature lists `IOSurfaceRootUserClient` under
`com.apple.security.iokit-user-client-class`. The v39 diagnostic requests only
that entry via [remote-layer-entitlements.plist](remote-layer-entitlements.plist).
A separately signed, unentitled copy supports `--context-only` so surface
allocation and context submission can be tested independently.

Automatic approval review initially rejected installing the new entitlement
and its trust hash. No part of that rejected installation ran. The user then
explicitly approved the requested guest change and authorized changes inside
the guest. The entitled diagnostic was installed after that approval; host
policy and original system-service entitlements were not changed.
The user authorization applies to subsequent guest-side research as well.

### V39 results

The [unentitled context-only run](evidence/remote-context-v39.txt) returned a
non-null remote CAContext with context ID 3741310610, then returned from
CATransaction commit/flush and exited 0. It did not allocate an IOSurface.
The first context call waited while backboardd completed startup. These
observations establish successful client-side creation/submission calls;
they do not by themselves prove that the server rendered or retained content.

The [IOSurface-entitled run](evidence/remote-layer-v39.txt) passed the exact
operation denied in v38:

```text
REMOTE_SURFACE_PRESENT=1
REMOTE_SURFACE_SIZE=16384 STRIDE=256
REMOTE_LOCK_RESULT=0
REMOTE_CONTEXT_PRESENT=1
REMOTE_CONTEXT_ID=2671984802
REMOTE_COMMIT_RETURNED
REMOTE_SERVER_RENDER_BEGIN
Alarm clock: 14            /bin/remote-layer-probe
REMOTE_EXIT=142
```

Thus actual IOSurface allocation, CPU mapping/writing, and unlocking work in
this guest with the scoped entitlement. The subsequent synchronous server
render did **not** return before the probe's 25-second alarm. No captured
pixels or render return value were obtained. The alarm terminated only the
client; QMP continued to report the guest running. This is a new server-side
investigation point, not evidence of a working remote compositor or a proven
GPU wait. Next diagnostic: sample the client's Mach wait and backboardd's
render threads while reproducing the request, then identify the first
blocking call. Do not replace or restart the guest merely because the
observation/probe timeout expired.

Build identities: [unentitled](evidence/remote-context-v39-build.json),
[IOSurface permission](evidence/remote-layer-v39-build.json), and
[v38 baseline](evidence/remote-layer-probe-build.json). Both v39 probes compile
from the same source; `codesign -s - --entitlements
research/remote-layer-entitlements.plist` signs the surface-enabled copy.
The unentitled copy's absent entitlements were checked before installation.
V38 was intentionally stopped for installation. Current v39 uses QMP
`/tmp/a19-ui-v39-qmp.sock` and UART `/tmp/a19-ui-v39-serial.sock`.

A [post-timeout check](evidence/remote-layer-post-v39.txt) found backboardd PID 26
with LAST_EXIT=0. CADisplay returned its five original zero-sized Wireless
displays, no main display, and exit 0. The render timeout did not block this
display-query path.


## Remote server rendering succeeds on retry (v39)

The planned live thread sample produced a different outcome: the render
request completed before the sampler could attach to the client. The
[successful request](evidence/remote-render-success-v39.txt) returned true,
locked the IOSurface, and compared every active pixel against opaque red:

```text
REMOTE_SERVER_RENDER_RESULT=1
REMOTE_READ_LOCK_RESULT=0
REMOTE_CHANGED_BYTES=16384 RED_PIXELS=4096
REMOTE_READ_UNLOCK_RESULT=0
RENDER_EXIT=0
```

The client exited before `task_for_pid`, explaining that sampler's RESULT=5.
Backboardd's sample succeeded and all sampled threads were resumed. These are
post-request thread candidates, not a trace of the original timeout.

Developer Mode had temporarily been enabled for thread inspection. To test
that confounder, LLDB restored the original guest state byte to zero, read it
back, and [detached](evidence/remote-render-debugger-detach-v39.txt). A second
[successful request](evidence/remote-render-devmode-off-v39.txt) first verified
`security.mac.amfi.developer_mode_status=0`, then again returned true and
counted 4,096 exact red pixels. Thus the override does not need to remain
active for this path. This does not prove why the first request timed out,
nor rule out one-time initialization or effects of prior guest execution.

Unlike the earlier local CPU tests, this probe uses a remote CAContext and
CARenderServerRenderLayer, whose inspected wrapper marshals a Mach request.
Actual IOSurface pixels now confirm the remote path can render this client's
solid-color layer without a registered GPU. No SpringBoard content or usable
main display is established by this result. Developer Mode is currently off;
no debugger is attached. QEMU's loopback GDB endpoint remains available for
further diagnostics at `127.0.0.1:63439`.

## SpringBoard reaches execution, then crashes (v40)

With both rendering paths producing pixels, the investigation moved to the
actual system application. A diagnostic copy of the original SpringBoard
launch plist retained its mobile user, program, and MachServices. KeepAlive
was disabled, `_PanicOnCrash` removed, and stdout/stderr redirected to
`/private/var/tmp/springboard.{stdout,stderr}`.

A 4,250-byte single-line transfer via the short-command UART helper lost data
and left Bash in a continuation prompt. It did not submit a valid job. V39 was
then intentionally stopped to install the plist through the disk image;
do not use that UART helper as a general long-line file uploader.

The [first v40 submission](evidence/springboard-launch-v40.txt) returned errno
137 before execution. The legacy launch helper submits into the mobile user
domain (`user/501`); launchd rejects `_Conclave` there with “Conclave can only
be set on LaunchDaemons or Extensions.” No stock launchctl was found in the
mounted restore, system, or cryptex trees.

Removing only `_Conclave` from that diagnostic plist allowed the
[next submission](evidence/springboard-no-conclave-v40.txt) to return errno 0.
SpringBoard PID 43 executed, then exited with SIGSEGV after 2,666 ms. Launchd
reported `PID=-1 LAST_EXIT=11`; stderr was empty. Backboardd remained PID 26
with LAST_EXIT=0. This is application execution, not successful initialization
or evidence of SpringBoard pixels. No automatic crash/restart loop was enabled.

The checked-in [springboard-probe.plist](springboard-probe.plist) now includes
the Conclave removal. [Configuration differences and hashes](evidence/springboard-launch-config.json)
record both the initial rejected variant and the updated diagnostic variant.
This mobile-domain probe is not a faithful system-daemon launch: the correct
bootstrap domain and Conclave setup remain separate integration work.
`HighPriorityIO` also produces a non-fatal unknown-key warning, and launchd
reports a missing type-6 persona. Neither has been established as the crash's
cause. Catch the synchronous SIGSEGV before diagnosing it as missing GPU,
display, resources, or service dependencies.

Current v40 is running on QMP `/tmp/a19-ui-v40-qmp.sock`, UART
`/tmp/a19-ui-v40-serial.sock`; Developer Mode has not been overridden and no
debugger is attached. `/launchjobs/com.apple.SpringBoard.probe.plist` in the
image is the initial Conclave-bearing version; the successfully submitted
version is `/private/var/tmp/springboard-no-conclave.plist`. Stage the updated
checked-in plist during the next intentional image update.

## SpringBoard's user-directory crash fixed; main-display assertion exposed (v41–v42)

The new `launch-probe start LABEL` control enables repeated managed launches
without recreating jobs or changing SpringBoard's execution identity.
`stop LABEL` and `remove LABEL` send their corresponding legacy launch messages
in the caller's current domain; they do not select the system domain. The
helper compiles with warnings as errors, remains unentitled, and was
[successfully tested restarting SpringBoard](evidence/launch-start-springboard-v42.txt).
Stop/remove have not yet been exercised. [Build identity](evidence/launch-control-v41-build.json).

A direct root launch was **not** an adequate reproduction of the mobile job:
it reached SIGTRAP while the managed job still had a SIGSEGV. The kernel
exception entry provided a better capture point. [V41 crash evidence](evidence/springboard-user-directory-v41.md)
shows `getpwuid(getuid())` returning null inside `BSCurrentUserDirectory`;
the unchecked directory-field access faults at address 0x30. Its caller is
ChronoServices initialization through SBChronoApplicationProcessStateObserver.

The diagnostic root lacked `/etc/passwd`. Root could resolve the mobile
account from `/etc/master.passwd`, masking the missing public database during
root-only checks. V42 adds only the matched system image's
`/private/etc/passwd`, mode 0644; the protected master database remains
unchanged. [File identity](evidence/public-user-database-v42.json) records its
size/hash without committing account database contents.

[Retest](evidence/springboard-passwd-retest-v42.txt) reaches a later failure.
A managed restart under the debugger identifies the explicit assertion:
**`Invalid condition not satisfying: mainDisplay`** in FBSDisplayMonitor,
called by FBDisplayManager and FBSystemShellInitialize.
[Debugger observations](evidence/springboard-main-display-v42.md) and
[symbolication](evidence/springboard-main-display-v42-symbols.json) preserve
the actual fault and call chain. This is now a demonstrated main-display
requirement, beyond the earlier observation that CADisplay.mainDisplay is nil.
The next integration step is a valid main display and usable mode in the
render server; suppressing this assertion would not establish either.

V40 and v41 were intentionally stopped for image updates after debugger
cleanup. Current v42 is running with QMP `/tmp/a19-ui-v42-qmp.sock`, UART
`/tmp/a19-ui-v42-serial.sock`, and a loopback GDB endpoint at
`127.0.0.1:63442`. No debugger is attached, all trace breakpoints were deleted,
and Developer Mode is unchanged/off. The updated Conclave-free SpringBoard
job is now installed in `/launchjobs/com.apple.SpringBoard.probe.plist`.
The staged RunningBoard job also has KeepAlive disabled for future bounded
startup diagnostics; RunningBoard has not been started in this experiment.

The [post-trace check](evidence/springboard-post-trap-v42.txt) confirms backboardd
still runs and CADisplay queries complete. The five Wireless displays remain
zero-sized and `CADISPLAY_MAIN_PRESENT=0`; SpringBoard is stopped with LAST_EXIT=5.


## Client-visible virtual main LCD (v42)

[Procedure and interpretation](evidence/virtual-main-v42.md) document two
small, temporary debugger changes in backboardd: enable the existing virtual
main-display path, then provide `LCD` as its constructor name. The default
`CAVirtualMainDisplay` name produces a supported internal display but does
not pass the client main-display name filter, which recognizes `LCD` or
`Internal`. Supplying `LCD` yields `CADISPLAY_MAIN_PRESENT=1` with a nonzero
416 × 496 mode. No fabricated main-display pointer or skipped assertion is
involved. Developer Mode stayed off.

SpringBoard then advances to FrontBoard workspace/process initialization,
where it asserts `invalid pid for <inert:[anon<SpringBoard>:-1]*>`.
Investigate RunningBoard and launch-domain registration next. The display
changes are process-local and must be repeated after restarting backboardd;
a maintained boot configuration and graphical presentation remain future work.


## RunningBoard advances startup to a missing CoreEmoji bundle (v42–v44)

RunningBoard starts from the staged matching job and its Mach service resolves.
With RunningBoard and the virtual LCD active, SpringBoard changes from the
inert-process-handle trap to SIGABRT. In v42, subsequent spawning hit an
unsupported SPTM opcode and a terminal nested panic; [observations and
limits](evidence/runningboard-start-v42.md) preserve that separate emulator
lead. No speculative opcode patch was applied.

A fresh v43 reproduced the selected virtual LCD, started RunningBoard, and
captured SpringBoard's actual stderr assertion:

```text
Assertion failed: (frameworkBundle && "CoreEmoji framework bundle could not be found."), function createFrameworkBundle_block_invoke, file CEMUtilityFunctions.cpp, line 276.
```

The missing on-disk CoreEmoji.framework bundle was then copied from the
matching 24A437 system image into the stopped disposable service-root image.
All 942 copied files were hash-compared to their source; [identity summary](
evidence/coreemoji-resource-manifest-v44.json) records the result. The shared
cache already provides framework code; this addition supplies its on-disk
bundle resources. Firmware contents are not committed. Retest after adding
the bundle remains pending at this checkpoint.

RemoveJob during an in-flight SpringBoard respawn returned errno 36 but launchd
subsequently terminated and removed the job. Always verify final job state;
the reply alone does not fully describe asynchronous cleanup.


## CoreEmoji retest reaches application launch (v44)

The virtual LCD was reapplied with v44 shared-cache slide `0xd6b0000`, and
fresh CADisplay inspection again selected LCD as main. RunningBoard started.
SpringBoard no longer reports the CoreEmoji bundle assertion. It now throws
`NSInternalInconsistencyException: Got nil for _systemAppInfo.`
[Stderr excerpt](evidence/springboard-coreemoji-retest-v44.txt) and
[symbolication](evidence/springboard-system-app-v44-symbols.json) show
`-[SBApplicationController initWithTelephonyStateProvider:]`,
`-[SpringBoard applicationDidFinishLaunching:]`, UIApplication's scene/launch
callbacks, and UIApplicationMain. This establishes progress into application
launch, not a functioning main loop or rendered home screen.

The guest lacks `/usr/libexec/lsd`; its LaunchServices mapdb/xpc lookups fail.
SpringBoard.app/Info.plist is present, so an absent application bundle plist
alone does not explain the missing record. V45 stages the untouched matching
lsd executable and `com.apple.launchservices.lsd.csdb` seed, with a
[diagnostic job](lsd-probe.plist) preserving mobile identity and MachServices.
RunAtLoad is enabled, KeepAlive disabled, and stdout/stderr redirected under
/var/tmp. [File identity](evidence/lsd-files-v45.json) records exact copies.
Service startup and database/application registration still need testing.
The missing service is a lead, not yet a verified fix for _systemAppInfo.


V45 LaunchServices submission succeeds and lsd executes, but exits with
SIGABRT in approximately 368 ms. Its stderr is empty. A second launch was
caught directly at libc `abort` (unslid `0x187f788b0`, v45 cache slide
`0x4fd8000`). [The stack](evidence/lsd-abort-v45-symbols.json) passes through
`_LSLazyLoadObjectOnQueue`, `-[_LSDefaults userContainerURL]`,
`-[_LSDefaults preSydroFSecurePreferencesFileURL]`, and `_LSServerMain`.
Repeated lookup failures for `com.apple.containermanagerd` immediately precede
this failure. This identifies per-user container initialization as the next
LaunchServices dependency; the existing system container helper is separate.
The debugger was detached and all breakpoints removed. V45 remains running;
virtual LCD and RunningBoard have not yet been applied in this boot.


## Per-user container agent preparation (v46)

V45 confirmed that only `/usr/libexec/containermanagerd_system` was installed;
`com.apple.containermanagerd` was absent. V46 adds the byte-identical 24A437
`/usr/libexec/containermanagerd` and a [diagnostic agent job](container-agent-probe.plist).
Original agent/fixed-user/proxy-bundle/proxy-system/kernel-upcall arguments and
MachServices are preserved. The diagnostic job explicitly selects mobile,
disables KeepAlive, removes _PanicOnCrash, and captures stdout/stderr in /var/tmp.
[File identity](evidence/container-agent-v46.json) records the untouched binary.
The existing system helper, writable /private/var and ICU resources remain.


The container-agent test succeeds: launchd lists containermanagerd PID 38 and
lsd PID 40, both with LAST_EXIT=0. A temporary Developer Mode byte override
allowed `thread-probe` to sample both tasks successfully. [Thread samples](
evidence/container-lsd-threads-v46.txt) and [symbolication](
evidence/container-lsd-threads-v46-symbols.json) show both in CFRunLoopRun;
lsd specifically reaches `runServerMainRunLoop` under LSServerMain. Developer
Mode was restored to zero and verified in the debugger immediately afterward.
No daemon executable, entitlement, or sandbox profile was patched.

The virtual LCD was reapplied with v46 cache slide `0xbbc0000` and selected by
CADisplay. RunningBoard was then started. SpringBoard still throws
`Got nil for _systemAppInfo.` even with lsd alive. Service availability alone
therefore does not supply the application record. Next investigate database
contents and application registration. The exact cache has a private
`_LSRegisterURL` routine at `0x186f01030`; its semantics and suitability for
this task have not yet been verified or exercised.

SpringBoard's crash-loop job was removed; the guest and its supporting
services remain running. The UART command helper now drains output while
sending each paced byte, rather than waiting for the complete input line.
Under the active crash-loop logging, RemoveJob completed with errno 0 and an
intact completion marker using this version; the earlier send-then-read
helper had truncated markers and observation timeouts. Python compilation
and diff checks also pass. This does not establish reliable large-file upload.


## Bounded application-registration probe (v47)

The matching CoreServices cache has
`-[LSApplicationWorkspace registerApplication:]` at `0x186f85c70`. Disassembly
shows it forwards its URL argument to `_LSRegisterURL(url, false)` and returns
whether the OSStatus is zero. This gives a runtime selector path without
calling a hardcoded private function address.

[ls-registration-probe.c](ls-registration-probe.c) drops root credentials to
mobile, loads CoreServices, checks selectors, and queries
`LSApplicationProxy applicationProxyForIdentifier:` for com.apple.springboard.
It prints proxy description, isInstalled and bundleURL where supported. Only
`--register-springboard` invokes registration, with the existing
/System/Library/CoreServices/SpringBoard.app URL. Query again in a separate
process to check actual resulting state; method success alone is insufficient.
The process has a 30-second alarm and no added entitlements.

Build with the matching restore libSystem (same flags as the other C probes),
ad-hoc sign, copy into the stopped guest image and merge the CDHash into the
version-1 trust cache. [Probe identity](evidence/ls-registration-probe-v47.json)
records the exact build and additive trust-cache entry count. No existing
entries were removed. Registration has not yet been claimed successful.


V47 boots successfully and both diagnostic service submissions return errno 0.
The first guest query has not executed: automatic approval review timed out
on the elevated UART command and on its one permitted retry. A normal-sandbox
attempt then failed to connect to the guest socket with EPERM. This is a
host-tool execution limitation, not evidence about LaunchServices or the
registration API. No application registration was attempted. The guest remains
booted with container/lsd jobs submitted; virtual LCD and RunningBoard have not
yet been reapplied on this boot. Retest the query before drawing conclusions.


### V47 registration execution and error capture

After renewed guest authorization, the UART tests executed successfully.
The [initial query](evidence/ls-query-before-v47.txt) returns an INVALID
SpringBoard proxy, isInstalled=0 and nil bundleURL. The explicit
[registration attempt](evidence/ls-register-v47.txt) returns false; a
[fresh process](evidence/ls-query-after-v47.txt) confirms the record remains
absent. This rules out treating a non-null proxy as an installed application.

[Debugger observations](evidence/ls-registration-error-v47.md) capture the
underlying NSOSStatusErrorDomain **-9499**, with LSRegistration.mm /
_LSRegisterBundleNode error metadata. The exact failing inner branch still
needs tracing; the code alone does not establish a missing service or an
entitlement denial. Both temporary hardware breakpoints were deleted and
the debugger detached. The guest remains running with no application record
created, and no SpringBoard/RunningBoard job started in this boot.


### Missing install information and v48 rebuild probe

V47 hardware breakpoints establish the exact failing path. At
`0x186e24094 + 0x3278000`, `_LSBundleFindWithNode` returns -9499.
Execution then reaches `0x186e241a8 + slide`, with x20=0,
x25=0x08000000 and x26=-9499, skipping the registration branch.
Its cold logging path refers to: "no install info, bundle at %@ will not be
registered" (0x186fde504). This is stronger evidence than an unexplained
OSStatus: the URL-only API lacks installation metadata for this bundle.

Do not switch blindly to `registerApplicationDictionary:`. In 24A437 its
implementation forwards to `registerApplicationDictionary:withObserverNotification:`,
which unconditionally returns false. Its diagnostic explicitly says this
interface can no longer register applications.

V48 adds `--rebuild-system` to the existing mobile-identity probe. It calls
`_LSPrivateRebuildApplicationDatabasesForSystemApps:internal:user:uid:` with
true/false/false/501, checks selector availability, and prints the returned
boolean. A 60-second alarm bounds the caller; a timeout does not establish
that server-side rebuilding stopped. Fresh-process installed-state queries
remain mandatory. Default query and URL registration modes are unchanged.

The image also contains the original matching installd binary and a
[diagnostic job](installd-probe.plist) preserving its `_installd` user/group,
arguments and Mach service, with RunAtLoad enabled, KeepAlive and pressured
exit disabled, and stdout/stderr captured in /var/tmp. Both account entries
exist in the guest. [Hashes and trust-cache counts](evidence/rebuild-probe-v48.json)
record the additions; no new probe entitlements. Runtime results follow.

V48 boots (cache slide 0x14e24000) and all three service submissions return
errno 0. Installd then exits repeatedly with OS_REASON_LIBSYSTEM:
"Failed to get installd daemon container", MIInstallerErrorDomain code 4,
underlying NSCocoaErrorDomain code 4099. Missing usermanagerd and
keybagd.UserManager lookups precede the failure, including repeated requests
from containermanagerd. A /var sandbox denial is also present; causality
between these messages has not yet been established.

The installed v48 rebuild probe first queries LSApplicationProxy and stalls
there, before LS_REBUILD_BEGIN. Source now skips that preliminary query for
rebuild mode, so a blocked query cannot prevent requesting a rebuild; this
adjustment compiles but is not yet installed in v48. The original firmware
usermanagerd job advertises BOTH missing UserManager Mach services and is the
next concrete dependency to investigate. No rebuild success is claimed.

The v48 probe exits via its 60-second SIGALRM while still in LS_QUERY_BEGIN;
LS_REBUILD_BEGIN never appears. This is a verified caller termination, not
an observer timeout or a completed rebuild.

The installd job was removed (errno 0 and launchd legacy-remove confirmed).
A [fresh query](evidence/ls-query-after-remove-v48.txt) then completes normally
and still reports SpringBoard INVALID/isInstalled=0/bundleURL=nil. V48 remains
running with container/lsd services; no debugger is attached.


### UserManager dependency experiment (v49)

V49 stages the byte-identical 24A437 /usr/libexec/usermanagerd and a
[diagnostic job](usermanagerd-probe.plist). The original root identity,
arguments (-t 15), environment and all three Mach services are preserved.
For legacy submission, LimitLoadToSessionType=System is removed; this changes
the bootstrap domain and must not be confused with normal system startup.
_PanicOnCrash is removed, KeepAlive and pressured exit are disabled, and
stdout/stderr are captured in /var/tmp. [Hashes](evidence/usermanagerd-v49.json)
also identify the installed probe, now skipping the preliminary application
query for --rebuild-system. No added entitlements or original daemon patches.

V49 UserManager executes but aborts with "Daemon failed to load persona
manifest." Static inspection identifies /private/var/keybags/persona.kb and
the version-1 keys UsePersonaManifestVersion, UsePersonaGenerationID and
UserPersonaDictionary. The keybags directory was absent. It was created live,
then a minimal XML manifest (version 1, generation 1, empty user dictionary)
was written only if absent. The equivalent [seed plist](persona-empty-manifest.plist)
is an experiment; runtime validation follows. These /var changes are volatile.

The initial v49 rebuild call segfaulted because the probe incorrectly passed
uid 501 by value. Disassembly at 0x186e712b8 dereferences this argument as
uint32_t*. Source now passes &uid and compiles cleanly. The installed v49
binary still has the old calling convention. A debugger-assisted attempt
replaced the argument with a stack pointer, but its write used LLDB's default
hexadecimal parsing: readback was 1281, not 501. That attempt ended in SIGALRM
and is invalid as a UID-501 rebuild test. No registration conclusion is drawn
from either diagnostic error. The temporary breakpoint was deleted and the
debugger detached.

The empty persona seed is accepted: after removing and resubmitting its job,
UserManager PID 66 remains listed with LAST_EXIT=0. Installd advances from
its initial dependency wait to MIInstallerErrorDomain code 4 with underlying
NSPOSIXErrorDomain code 2 (ENOENT), SourceFileLine=207. The exact missing
container path is not yet established. [Observed transitions](evidence/persona-seed-result-v49.txt).
The fresh SpringBoard query still ends via SIGALRM while installd crash-loops.

V49 subsequently hits the [same SPTM panic](evidence/sptm-panic-v49.txt) at
0xfffffe00071024e0 (ESR 0x02000000) seen in v42. The attempted installd
RemoveJob does not complete. The log ends with nested panic limit/reset-or-spin;
this is a verified terminal guest failure, not an observation timeout. Preserve
the persona seed in /var-seed/keybags for the next boot, along with the corrected
UID-pointer probe. Do not claim the v49 guest remains usable.

V50 image preparation is complete: the seed is persisted at
/var-seed/keybags/persona.kb and the corrected UID-pointer probe is installed
and trusted. [Exact hashes](evidence/persona-seed-image-v50.json) identify both.
The image is detached. V50 has not yet been booted; the next test should start
UserManager and the container/lsd services, then test rebuild authorization
before resubmitting the known-failing installd job. The rebuild server checks
com.apple.lsapplicationworkspace.rebuildappdatabases (static inspection); the
probe currently has no such entitlement.


### Corrected rebuild call and authorization (v50)

V50 boots with cache slide 0x121b0000. The persisted empty persona manifest
works across reboot: UserManager PID 38 is listed LAST_EXIT=0 alongside
containermanagerd PID 40 and lsd PID 42. Installd is initially absent.
The corrected UID-pointer probe completes without a crash and returns false.
A hardware breakpoint at 0x186ee6528 + slide captures x26=0 immediately after
_LSCheckEntitlementForXPCConnection for
com.apple.lsapplicationworkspace.rebuildappdatabases.

For one diagnostic call, x26 was changed to 1 and read back, then the
breakpoint was deleted and debugger detached. This permits the rebuild
operation rather than fabricating its result. The API returns true, but the
[fresh query](evidence/query-after-rebuild-v50.txt) still reports SpringBoard
INVALID, isInstalled=0 and nil bundleURL. Installd/fshelper lookup failures
remain. This proves API success is insufficient for the intended database
contents. [Baseline](evidence/rebuild-v50.txt) and
[authorized diagnostic call](evidence/rebuild-auth-v50.txt).

A [single-entitlement plist](ls-rebuild-entitlements.plist) is prepared for a
reproducible signed probe; it has not yet been installed/tested. The current
guest probe is still unentitled and no debugger is attached.

Creating /private/var/installd and chowning it to 33:33 does not fix the
startup failure: the original installd still exits with underlying ENOENT,
SourceFileLine=207. [Failure](evidence/installd-home-result-v50.txt) and
[service listing](evidence/installer-state-v50.txt) rule out the simple missing
home-directory hypothesis. This change is only in volatile /var, not the image.

Further static landmarks: InstalledContentLibrary implements
+[MIMCMContainer daemonContainerForIdentifier:personaUniqueString:error:]
at 0x1ab0b4220, its block at 0x1ab0b4420, and
+[MIMCMContainer daemonContainerForPersona:error:] at 0x1ab0b44a8.
The block requests class 10 (daemon) with creation enabled. This is distinct
from the account home. InstalledContentLibrary symbols are available via a
scoped ipsw dyld symaddr dump; a whole-cache exact lookup for the wrong class
MIUserManagement wastes several minutes and finds nothing.

Installd RemoveJob returns errno 0. V50 remains running, with the supporting
UserManager/container/lsd jobs and no debugger attached. No virtual LCD,
RunningBoard or SpringBoard job has been applied in this boot.


### Installer failure is an empty persona list (v50)

[Direct error inspection](evidence/installd-empty-personas-v50.md) resolves
the misleading ENOENT: the MIInstallerErrorDomain code-4 error says
"UserManager returned an empty persona list", from
-[MIUserManagement _onQueue_refreshPersonaInformationWithError:].
The empty manifest starts UserManager but is insufficient to initialize
its consumers. The next required state is actual default personas, not an
arbitrary container directory. The temporary breakpoint was removed and
debugger detached; installer RemoveJob returned errno 0.

The stock executable supports --init (entry dispatch at 0x1000099ac calls
0x10005d438). A [live test](evidence/usermanager-init-v50.txt) reaches early
boot setup, sees EAPFS in the device tree, then APFSContainerGetBootDevice
fails with 49154. UserManager explicitly triggers a userspace panic:
FAILED TO FIND DISKNODE. This is separate from the prior SPTM instruction
panic. The log reaches nested-panic-limit/reset-or-spin. Do not repeat --init
unchanged on this RAM-disk layout or claim it created personas.

Potential next directions are provisioning a valid default-persona manifest
from the matching implementation, or supplying the APFS boot-volume state
required by stock initialization. The original daemon's manifest constructors
and parsing methods are visible through ipsw macho info --objc --verbose.


### Populated persona seed and missing caller identity (v51)

The [experimental seed generator](make-persona-seed.py) reproduces the v1
manifest nesting from the matching usermanagerd parser: UserPersonaDictionary
maps user UUID strings to data containing a NUMENT/BLOB dictionary; BLOB is
data containing an array of persona dictionaries. The system-session
constructor uses types 3 and 5, while the user constructor uses 0, 3, 5, 4.
UUIDs and kernel IDs in this test are synthetic; generating the file does
not allocate kernel personas or provision APFS volumes. The empty seed is
retained as a separate earlier experiment.

V51 starts UserManager with this populated seed, but stock installer startup
still fails at SourceFileLine 207. Debugging reveals its request uses the
embedded, user-specific persona-list method, whose caller-to-user resolver
returns nil. A one-shot substitution of the existing mobile user string
makes the daemon's lookup return **four personas**, verified at the actual
array-count return. This confirms the seed reaches live daemon state.
Nevertheless, the installer still fails afterward, including when the
system-session UUID is substituted instead. The next investigation is the
client response/conversion into persona attributes and subsequent lookups,
alongside proper kernel persona/session initialization.

[Exact debugger observations and image hash](evidence/persona-debug-v51.md),
[baseline installer start](evidence/persona-installer-start-v51.txt),
[system-user diagnostic result](evidence/persona-system-lookup-result-v51.txt),
and [mobile-user diagnostic result](evidence/persona-mobile-result-v51.txt).
The normal SpringBoard query still returns INVALID/isInstalled=0/nil URL.
The guest-only developer override was restored, all breakpoints removed,
and the debugger detached. V51 is running with the installer job removed;
no interactive SpringBoard or touch input is available yet.


### Repeated SPTM panic: experimental invalidation workaround (v52)

The [persona-client follow-up](evidence/persona-client-followup-v51.md) did
not resolve installer startup. V51 then reached the same terminal SPTM panic
as earlier runs. Original-file comparison confirms the fault word 0x0020134d
is present in Apple's SPTM, not runtime corruption from these experiments.
Its control-flow context suggests ASID-generation invalidation: x13 contains
a low-byte index and high-byte generation; the faulting branch is taken when
the stored generation differs, near TTBR0_EL1/TCR_EL1 updates. Exact custom
instruction semantics remain unverified.

A [hash-guarded experimental patcher](patch-sptm-invalidation-probe.py)
substitutes architectural `tlbi vmalle1is` at that single site. This is a
conservative full EL1 TLB invalidation hypothesis, not an upstream-ready
emulation implementation. The boot wrapper accepts an explicit SPTM override
and continues to use the original by default:

```sh
python3 research/patch-sptm-invalidation-probe.py \
  firmware/iphone-17-ui-probe/sptm \
  firmware/iphone-17-ui-probe/sptm-tlbi-probe
# Add to the existing v51 boot environment:
SPTM=firmware/iphone-17-ui-probe/sptm-tlbi-probe
```

V52 boots and completes **300 consecutive process launches**. A hardware
breakpoint proves execution reached the substituted instruction with
x13=0x102, x24=0, x25=1, matching the earlier fault conditions. After the
breakpoint was removed and debugger detached, execution continued through
PID 336 and printed PROCESS_STRESS_COMPLETED=300. [Exact observations and
hashes](evidence/sptm-invalidation-v52.md), [stress transcript](evidence/sptm-process-stress-v52.txt),
and [post-test shell check](evidence/sptm-post-stress-v52.txt).

This makes further service experiments more practical; it does not prove
complete translation correctness or restore the graphical UI. V52 remains
running with no debugger attached. The installer, lsd, and UserManager probe
jobs have not been submitted in this boot. The populated manifest remains
in the disk seed. No host security settings or QEMU submodule source changed.
