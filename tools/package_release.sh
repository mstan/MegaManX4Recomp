#!/usr/bin/env bash
# Stage an already-built production runtime through the shared release gates.
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
fw=${PSXRECOMP_ROOT:-"$root/psxrecomp-v4"}
exec bash "$fw/tools/package_game_release.sh" --root "$root" \
    --zip-prefix MegaManX4Recomp --exe-name MegaManX4Recomp \
    --display-name "Mega Man X4 Recompiled" --runtime-target psx-runtime \
    --runtime-file input.ini --runtime-file START_HERE.txt \
    --doc README.md --doc LICENSE --doc RELEASE_NOTES.md \
    --ship-without-overlay-cache-because "Original-disc inventory found no separate X4 executable overlay; the complete resident boot program is statically compiled. Eligible uncovered dirty-RAM code can use the bundled native toolchain. No private capture or unqualified cache is shipped." \
    "$@"
