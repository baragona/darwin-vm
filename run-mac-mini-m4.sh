#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
export FIRMWARE_DIR=firmware/mac-mini-m4
exec ./run.sh "$@"
