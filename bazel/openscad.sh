#!/usr/bin/env bash
set -euo pipefail
export QT_QPA_PLATFORM=offscreen

if (($# == 0)); then
  echo "usage: openscad.sh /path/to/openscad.AppImage [OpenSCAD arguments...]" >&2
  exit 2
fi

openscad_appimage="$1"
shift
exec "$openscad_appimage" --appimage-extract-and-run --hardwarnings "$@"
