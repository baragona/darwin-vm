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

The full filesystem image from the same IPSW is being extracted locally to
inspect SpringBoard, backboardd, and their launch/service dependencies:

```sh
ipsw extract --remote \
  'https://updates.cdn-apple.com/2026FallFCS/b18c9502-c21d-4555-9bf7-21f3a238e6d7/iPhone18,3_27.0_24A437_Restore.ipsw' \
  --dmg fs --output ipsw_db/iphone-17-full --flat --json
```

Next: inspect that filesystem, map the minimum userspace dependencies, and
decide whether a basic display-service shim or more faithful display-controller
emulation gives the most useful next boot experiment. The interactive text
console is infrastructure for that work, not completion of the graphical goal.

## Sources

- [XNU boot video initialization](https://github.com/apple-oss-distributions/xnu/blob/main/pexpert/arm/pe_init.c)
- [XNU video console](https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/console/video_console.c)
- [XNU serial mode definitions](https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/console/serial_protos.h)

These public sources informed the experiments; the actual behavior above was
verified on the requested iOS 27 guest. Local logs and firmware stay untracked.
