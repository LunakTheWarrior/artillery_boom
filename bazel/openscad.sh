#!/usr/bin/env bash
set -euo pipefail
export QT_QPA_PLATFORM=offscreen
exec openscad --hardwarnings "$@"
