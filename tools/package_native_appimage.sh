#!/usr/bin/env bash
# Historical entrypoint; releases now use the same fresh shared build flow.
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
exec bash "$root/tools/package_appimage.sh" "$@"
