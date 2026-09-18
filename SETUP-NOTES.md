# M4 Mac mini and iPhone 17 setup

Prepared on 2026-09-17 in `/Users/kevin/Desktop/darwinvm` on an arm64 Mac
running macOS 26.6.2 (25G83), with 48 GiB RAM.

This project boots a minimal Darwin root shell, with a modified restore ramdisk.
It does not boot the macOS desktop or iPhone SpringBoard.

## Versions

- Upstream darwin-vm: `1c1b2c500e2192d29ef84941cd3b517024450526`.
- qemu-sptm submodule: `2867d847d3471560e773120ee50c42dbcbb6d60b`.
- Homebrew `ipsw`: 3.1.716.

| Target | Device | Board | Chip | Kernel collection | OS build |
| --- | --- | --- | --- | --- | --- |
| M4 Mac mini | Mac16,10 | j773gap | t8132 | mac16g | macOS 27.0, 26A428 |
| iPhone 17 | iPhone18,3 | v57ap | t8150 | v57 | iOS 27.0, 24A437 |

## Fork and dependencies

```sh
gh repo fork jprx/darwin-vm --clone=false
git clone https://github.com/baragona/darwin-vm.git
cd darwin-vm
HOMEBREW_NO_AUTO_UPDATE=1 brew install ipsw
git submodule update --init
```

The host already had jq, Python 3, Ninja, pkg-config, glib, pixman, and Apple
Command Line Tools. A fresh host may need `brew install jq python ninja pkg-config glib pixman`.

## Build QEMU

```sh
mkdir -p qemu-sptm/build logs
cd qemu-sptm/build
../configure --target-list=aarch64-softmmu --disable-pvg > ../../logs/qemu-configure.log 2>&1
make -j8 > ../../logs/qemu-build.log 2>&1
cd ../..
```

## Prepare both firmware sets

The fork adds `FW_DIR` and `IPSW_BIN` environment overrides to `get_files.sh`,
and `FIRMWARE_DIR` and `QEMU` overrides to `run.sh`. Original defaults still work.
Separate directories prevent one target overwriting the other.

Apple IPSW URLs were resolved using the ipsw.me device API for the exact build
numbers. The extraction tool downloads selected components using remote ZIP
access, so a full 12–26 GB IPSW download is unnecessary.

```sh
DEVNAME='Mac16,10' \
URL='https://updates.cdn-apple.com/2026FallFCS/afcfc88e-bbe6-44bf-a5da-07c56eebc06c/UniversalMac_27.0_26A428_Restore.ipsw' \
FW_DIR=firmware/mac-mini-m4 IPSW_BIN=ipsw_db/mac-mini-m4 \
./get_files.sh > logs/setup-mac-mini-m4.log 2>&1

DEVNAME='iPhone18,3' \
URL='https://updates.cdn-apple.com/2026FallFCS/b18c9502-c21d-4555-9bf7-21f3a238e6d7/iPhone18,3_27.0_24A437_Restore.ipsw' \
FW_DIR=firmware/iphone-17 IPSW_BIN=ipsw_db/iphone-17 \
./get_files.sh > logs/setup-iphone-17.log 2>&1
```

The upstream scripts extract the kernel collection, SPTM, TXM, device tree, and
restore ramdisk; patch the device tree; replace launch daemons with a root shell;
install/sign the included iOS command-line tools for the iPhone; and generate
a trust cache. Kernel log silencing is optional and was not applied.

Fix ownership inside each ramdisk (requires a host administrator password):

```sh
./fix_perms.sh firmware/mac-mini-m4/ramdisk.dmg
./fix_perms.sh firmware/iphone-17/ramdisk.dmg
```

Answer `y` at each confirmation. The `sudo chown` operations target the mounted
guest ramdisk's `bin`, `System`, and, if present, `libexec` directories.

## Run

```sh
./run-mac-mini-m4.sh
# In a second terminal:
./run-iphone-17.sh
```

Each VM gets 8 GiB of emulated memory. Exit with Ctrl-A followed by X.
The iPhone's MIE emulation can be slower. Firmware, downloaded components,
local build artifacts, and raw logs are not committed to this fork.

## Validation

- QEMU compiled successfully and reports version 11.1.0; `-machine help`
  lists the `darwin` machine.
- Both `get_files.sh` runs completed successfully and detached their ramdisks.
- Both targets have a kernel collection, device tree, SPTM, TXM, ramdisk,
  generated trust cache, and source URL record in `firmware/<target>/info`.
- Shell syntax checks and `git diff --check` passed.
- The host required a password for `sudo`; the owner ran both ownership fixes
  in a local terminal before boot verification.
- Both VMs booted simultaneously to interactive root shells, and executed
  `uname -v`, `id`, and `cat /System/Library/CoreServices/SystemVersion.plist`.

| Check | M4 Mac mini | iPhone 17 |
| --- | --- | --- |
| Shell prompt | `bash-3.2#` | `bash-5.3#` |
| Kernel | `xnu-13432.1.9~1/RELEASE_ARM64_T8132` | `xnu-13432.2.10~2/RELEASE_ARM64_T8150` |
| Darwin version | 27.0.0 | 27.0.0 |
| ProductVersion | 27.0 | 27.0 |
| ProductBuildVersion | 26A428 | 24A437 |
| Identity | `uid=0(root) gid=0(wheel)` | `uid=0(root) gid=0(wheel)` |
| Trust-cache entries | 546 | 470 |

Local serial transcripts are `logs/boot-mac-mini-m4.log` and
`logs/boot-iphone-17.log`, captured with:

```sh
script -q logs/boot-mac-mini-m4.log ./run-mac-mini-m4.sh
script -q logs/boot-iphone-17.log ./run-iphone-17.sh
```

Observed limitations and tips:

- `sw_vers` is absent in these minimal ramdisks. Read
  `/System/Library/CoreServices/SystemVersion.plist` to check the build.
- The restore root filesystem boots read-only; `/tmp` has no usable backing
  directory. A write smoke check therefore failed, while normal command
  execution and file reads succeeded. To add programs, use the upstream
  host-side ramdisk editing and trust-cache procedure.
- Boot logs contain missing-service/hardware warnings, and iPhone SEP timeout
  messages continue after the shell starts. These did not prevent the tested
  commands from running.
- Sending a long command to QEMU's serial console in a single automated write
  lost characters. Verification used short commands or eight-character chunks
  with 250 ms pauses. Type normally in an interactive terminal.
