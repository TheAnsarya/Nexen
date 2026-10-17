#!/bin/bash
# AppRun wrapper - ensures bundled native libraries are found before system ones
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="${HERE}/usr/bin:${HERE}/usr/lib:${LD_LIBRARY_PATH}"
exec "${HERE}/usr/bin/Nexen" "$@"
