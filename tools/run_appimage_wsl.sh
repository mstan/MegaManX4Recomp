#!/usr/bin/env bash
# Launch the packaged game under WSL with Linux-native, correctly quoted paths.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
# shellcheck source=/dev/null
. "$root/packaging/release/app.conf"
version=$(tr -d ' \t\r\n' < "$root/packaging/release/VERSION")
appimage="$root/release-linux/${EXE_NAME}-${version}-linux-x86_64.AppImage"
disc="$root/mmx4/Mega Man X4.cue"
data_dir=

while (($#)); do
    case "$1" in
        --appimage) appimage=$2; shift 2 ;;
        --disc) disc=$2; shift 2 ;;
        --data-dir) data_dir=$2; shift 2 ;;
        -h|--help)
            echo "usage: $0 [--appimage FILE] [--disc FILE] [--data-dir DIR]"
            exit 0
            ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

case "$appimage" in
    [A-Za-z]:[/\\]*) appimage=$(wslpath -u "$appimage") ;;
esac
case "$disc" in
    [A-Za-z]:[/\\]*) disc=$(wslpath -u "$disc") ;;
esac

[ -x "$appimage" ] || { echo "AppImage is missing or not executable: $appimage" >&2; exit 1; }
[ -f "$disc" ] || { echo "Disc CUE is missing: $disc" >&2; exit 1; }

export "${ENV_PREFIX}_DISC=$disc"
if [ -n "$data_dir" ]; then
    mkdir -p "$data_dir"
    export "${ENV_PREFIX}_DATA_DIR=$data_dir"
fi

exec "$appimage" --appimage-extract-and-run
