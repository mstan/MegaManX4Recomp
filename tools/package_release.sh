#!/usr/bin/env bash
# Compatibility entrypoint; all build, PGO, staging and gates are shared.
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
framework=${PSXRECOMP_ROOT:-"$root/psxrecomp-v4"}
exec bash "$framework/tools/package_title_release.sh" --root "$root" --jobs 2 "$@"
