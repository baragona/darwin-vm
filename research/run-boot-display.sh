#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${FIRMWARE_DIR:=firmware/iphone-17-baseline}"
: "${QMP_SOCKET:=/tmp/a19-display-qmp.sock}"
: "${SERIAL:=stdio}"
: "${DISPLAY_BACKEND:=none}"
: "${RAMDISK:=$FIRMWARE_DIR/ramdisk.dmg}"
: "${BOOTKC:=$FIRMWARE_DIR/bootkc}"
: "${DTREE:=$FIRMWARE_DIR/dtree}"
: "${MEMORY:=8G}"
: "${BOOT_ARGS:=rd=md0 serial=2 -v -noprogress wdt=-1 wlan-olyhal-abort}"
exec ./qemu-sptm/build/qemu-system-aarch64 \
  -M darwin,boot-display=on -m "$MEMORY" \
  -bootkc "$BOOTKC" -dtree "$DTREE" \
  -tc "$FIRMWARE_DIR/ramdisk.tc" -ramdisk "$RAMDISK" \
  -sptm "$FIRMWARE_DIR/sptm" -txm "$FIRMWARE_DIR/txm" \
  -args "$BOOT_ARGS" -display "$DISPLAY_BACKEND" -serial "$SERIAL" -monitor none \
  -qmp "unix:$QMP_SOCKET,server=on,wait=off" "$@"
