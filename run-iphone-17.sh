#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
export FIRMWARE_DIR=firmware/iphone-17
exec ./run.sh "$@"
