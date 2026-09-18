# A19 / iOS 27 graphics investigation

Goal: a usable graphical A19/iOS 27 guest. This experiment establishes a boot
framebuffer and UART input path; SpringBoard, touch/HID, accelerated graphics,
and a full installed system are not working yet.

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
relevant drivers, but they cannot be assumed to attach. Next work is to get
the shared cache usable, then investigate actual service/driver failures.

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
