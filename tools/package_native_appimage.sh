#!/usr/bin/env bash
# Wrap an already-built, versioned Linux runtime using shared package gates.
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
fw=${PSXRECOMP_ROOT:-"$root/psxrecomp-v4"}
build=; emitter=; output=; jobs=2
while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-dir) build=$2; shift 2 ;;
        --recompiler-build) emitter=$2; shift 2 ;;
        --output) output=$2; shift 2 ;;
        --jobs) jobs=$2; shift 2 ;;
        *) echo "usage: $0 --build-dir DIR --recompiler-build DIR --output FILE.AppImage [--jobs N]" >&2; exit 2 ;;
    esac
done
[ -n "$build" ] && [ -n "$emitter" ] && [ -n "$output" ]
if ! grep -qx 'PSX_NETPLAY:BOOL=ON' "$build/CMakeCache.txt"; then
    echo "X4 co-op AppImage requires a build configured with -DPSX_NETPLAY=ON" >&2
    exit 1
fi
version=$(tr -d '[:space:]' < "$root/packaging/release/VERSION")
cp "$root/packaging/release/game.toml" "$build/game.toml"
cp "$root/packaging/release/input.ini" "$build/input.ini"
cp "$root/packaging/release/START_HERE.txt" "$build/START_HERE.txt"
RELEASE_VERSION="$version" bash "$root/tools/package_release.sh" \
    --build-dir "$build" --recompiler-build "$emitter" --artifact linux-x64 \
    --stage-only --exclude-dev-mods
metadata=$build/appimage-metadata
mkdir -p "$metadata"
sed -e "s/@VERSION@/$version/g" -e 's/@APP_NAME@/Mega Man X4 Recompiled/g' \
    -e 's/@EXE_NAME@/MegaManX4Recomp/g' -e 's/@PAYLOAD_DIR@/megamanx4recomp/g' \
    -e 's/@ENV_PREFIX@/MMX4_RECOMP/g' "$root/packaging/linux/AppRun" > "$metadata/AppRun"
convert "$root/recomp/launcher/boxart.tga" -resize 240x240 -background transparent \
    -gravity center -extent 256x256 "$metadata/icon.png"
exec bash "$fw/tools/package_appimage.sh" \
    --payload "$root/dist/stage-game-linux-x64" --exe-name MegaManX4Recomp \
    --payload-name megamanx4recomp --app-run "$metadata/AppRun" \
    --desktop-file "$root/packaging/linux/io.github.mstan.MegaManX4Recomp.desktop" \
    --icon "$metadata/icon.png" --output "$output" --jobs "$jobs"
