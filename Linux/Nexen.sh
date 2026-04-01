#!/bin/bash
# Nexen launcher - ensures bundled native libraries (SkiaSharp, HarfBuzz) are
# found before any system-installed versions.
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="${HERE}:${LD_LIBRARY_PATH}"
exec "${HERE}/Nexen" "$@"
