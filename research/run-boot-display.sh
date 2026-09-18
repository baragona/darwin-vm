#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${FIRMWARE_DIR:=firmware/iphone-17-baseline}"
: "${QMP_SOCKET:=/tmp/a19-display-qmp.sock}"
: "${SERIAL:=stdio}"
: "${DISPLAY_BACKEND:=none}"
: "${BOOT_ARGS:=rd=md0 serial=2 -v -noprogress wdt=-1 wlan-olyhal-abort}"
exec ./qemu-sptm/build/qemu-system-aarch64 \
  -M darwin,boot-display=on -m 8G \
  -bootkc "$FIRMWARE_DIR/bootkc" -dtree "$FIRMWARE_DIR/dtree" \
  -tc "$FIRMWARE_DIR/ramdisk.tc" -ramdisk "$FIRMWARE_DIR/ramdisk.dmg" \
  -sptm "$FIRMWARE_DIR/sptm" -txm "$FIRMWARE_DIR/txm" \
  -args "$BOOT_ARGS" -display "$DISPLAY_BACKEND" -serial "$SERIAL" -monitor none \
  -qmp "unix:$QMP_SOCKET,server=on,wait=off" "$@"
