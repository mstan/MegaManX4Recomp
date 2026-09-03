#!/usr/bin/env bash
# Build a reproducible native Linux x86_64 AppImage. Safe to invoke from WSL
# even when this checkout lives on a Windows drive.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
version=""
out_dir=""
skip_build=0
build_dir=${BUILD_DIR:-"$root/build-appimage"}
cores=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
jobs=${BUILD_JOBS:-$(( cores > 4 ? cores - 2 : 2 ))}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --version) version=$2; shift 2 ;;
        --out) out_dir=$2; shift 2 ;;
        --build-dir) build_dir=$2; shift 2 ;;
        --jobs) jobs=$2; shift 2 ;;
        --skip-build) skip_build=1; shift ;;
        -h|--help)
            echo "usage: $0 [--version VERSION] [--out DIR] [--build-dir DIR] [--jobs N] [--skip-build] "
            exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

version=${version:-$(tr -d ' \t\r\n' < "$root/packaging/release/VERSION")}
# shellcheck source=/dev/null
. "$root/packaging/release/app.conf"
for name in APP_NAME EXE_NAME PAYLOAD_DIR DESKTOP_ID ENV_PREFIX ICON_SOURCE \
            EXPECTED_STATIC_SHARDS FRAMEWORK_DIR; do
    eval "value=\${$name:-}"
    [ -n "$value" ] || { echo "packaging/release/app.conf does not set $name" >&2; exit 1; }
done

to_unix_path() {
    case "$1" in
        [A-Za-z]:[/\\]*)
            command -v wslpath >/dev/null 2>&1 ||
                { echo "cannot translate Windows path '$1': wslpath is unavailable" >&2; exit 2; }
            wslpath -u "$1" ;;
        *) printf '%s\n' "$1" ;;
    esac
}
out_dir=${out_dir:-"$root/release-linux"}
out_dir=$(to_unix_path "$out_dir")
mkdir -p "$out_dir"
out_dir=$(CDPATH= cd -- "$out_dir" && pwd)
output=$out_dir/$EXE_NAME-$version-linux-x86_64.AppImage

# AppDir needs real Linux symlinks and mode bits. DrvFs cannot guarantee them.
stage_base=$build_dir
case "$root" in
    /mnt/*)
        if grep -qiE 'microsoft|wsl' /proc/version 2>/dev/null; then
            stage_base=${TMPDIR:-/tmp}/$PAYLOAD_DIR-appimage.$$
            echo "WSL DrvFs checkout: staging AppDir on Linux filesystem at $stage_base"
        fi ;;
esac
appdir=$stage_base/AppDir
tools_dir=${RECOMP_APPIMAGE_TOOLS:-"${XDG_CACHE_HOME:-$HOME/.cache}/recomp-appimage-tools"}

cleanup() {
    case "$stage_base" in
        /tmp/"$PAYLOAD_DIR"-appimage.*) rm -rf -- "$stage_base" ;;
    esac
}
trap cleanup EXIT HUP INT TERM

if [ -z "${SOURCE_DATE_EPOCH:-}" ]; then
    SOURCE_DATE_EPOCH=$(git -C "$root" log -1 --format=%ct 2>/dev/null ||
        stat -c %Y "$root/packaging/release/VERSION")
fi
export SOURCE_DATE_EPOCH
echo "version=$version SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH"

# A partial generated checkout can still configure and then quietly omit game
# code. Pin the expected split count and ensure every shard is in the CMake
# build graph.
static_count=$(find "$root/generated" -maxdepth 1 -name 'SLUS_005.61_full_*.c' | wc -l)
if [ "$static_count" -ne "$EXPECTED_STATIC_SHARDS" ]; then
    echo "expected $EXPECTED_STATIC_SHARDS generated static shards, found $static_count" >&2
    exit 1
fi
[ -f "$root/generated/SLUS_005.61_dispatch.c" ] ||
    { echo "missing generated dispatch source" >&2; exit 1; }

fw=$root/$FRAMEWORK_DIR
# Shared release staging surface: tag derivation, cache selection, toolchain
# staging, and mod catalog checks live in psxrecomp, not this title packager.
# shellcheck source=/dev/null
. "$fw/tools/release_overlay_stage.sh"
psx_release_stage_init "$fw"
generator=Ninja
command -v ninja >/dev/null 2>&1 || generator="Unix Makefiles"
bios_build=${PSXRECOMP_BIOS_BUILD:-recompiler/build-linux}
recompiler_bin=$fw/$bios_build/psxrecomp-game

if [ "$skip_build" = 0 ]; then
    # The recompiler is both the shard producer and the authority for the
    # packaged config hash. Always rebuild it against this pinned framework.
    cmake -S "$fw/recompiler" -B "$fw/$bios_build" -G "$generator" \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build "$fw/$bios_build" --target psxrecomp-game -j "$jobs"

    # Regenerate redistributable OpenBIOS on every release cut. Its generated
    # backend carries an emitter fingerprint; merely checking that the files
    # exist can silently link a backend from an older framework revision.
    cmake --build "$fw/$bios_build" --target psxrecomp-bios -j "$jobs"
    (cd "$fw" && PSXRECOMP_BIOS_BUILD="$bios_build" \
        tools/regen_bios.sh --config bios/OpenBIOS.toml)

    cmake -S "$root" -B "$build_dir" -G "$generator" \
        -DCMAKE_BUILD_TYPE=Release \
        -DPSX_SDL_BACKEND=SDL2 \
        -DPSX_DEBUG_TOOLS=OFF \
        -DCMAKE_EXE_LINKER_FLAGS="-Wl,--build-id=none"
    cmake --build "$build_dir" --target psx-runtime -j "$jobs"
fi

[ -x "$recompiler_bin" ] ||
    { echo "missing Linux recompiler: $recompiler_bin" >&2; exit 1; }
elf=$build_dir/$EXE_NAME
[ -f "$elf" ] || elf=$build_dir/psx-runtime
[ -f "$elf" ] || { echo "no runtime ELF under $build_dir" >&2; exit 1; }
file -b "$elf" | grep -q ELF || { echo "$elf is not an ELF binary" >&2; exit 1; }

# CMake's manifest is the final proof that every generated split was compiled,
# not merely present in generated/.
for shard in "$root"/generated/SLUS_005.61_full_*.c; do
    grep -Fq "$(basename "$shard")" "$build_dir/build.ninja" ||
        { echo "static shard absent from build graph: $(basename "$shard")" >&2; exit 1; }
done
echo "Verified $static_count static shards in the Linux build graph"

player_toml=$root/packaging/release/game.toml
game_id=$(sed -n 's/^[[:space:]]*id[[:space:]]*=[[:space:]]*"\([^"]*\)".*/\1/p' \
    "$player_toml" | head -1)
cg_tag=$(psx_overlay_cg_tag \
    --runtime-include "$fw/runtime/include" \
    --recompiler "$recompiler_bin" \
    --game-toml "$player_toml" \
    --flavor-from-build "$build_dir" \
    --runtime-target psx-runtime)
[ -n "$cg_tag" ] || { echo "could not compute codegen tag" >&2; exit 1; }
echo "game=$game_id  codegen tag=$cg_tag"
rm -rf -- "$appdir"
mkdir -p "$appdir/usr/bin" "$appdir/usr/share/$PAYLOAD_DIR"
payload=$appdir/usr/share/$PAYLOAD_DIR
install -m 0755 "$elf" "$appdir/usr/bin/$EXE_NAME"
sed -e "s|@VERSION@|$version|g" -e "s|@APP_NAME@|$APP_NAME|g" \
    -e "s|@EXE_NAME@|$EXE_NAME|g" -e "s|@PAYLOAD_DIR@|$PAYLOAD_DIR|g" \
    -e "s|@ENV_PREFIX@|$ENV_PREFIX|g" \
    "$root/packaging/linux/AppRun" > "$appdir/AppRun"
chmod 0755 "$appdir/AppRun"
install -m 0644 "$root/packaging/linux/$DESKTOP_ID.desktop" \
    "$appdir/$DESKTOP_ID.desktop"

for tree in assets bios; do
    [ -d "$build_dir/$tree" ] || { echo "build did not stage $tree/" >&2; exit 1; }
    cp -a "$build_dir/$tree" "$payload/$tree"
done
psx_add_mod_catalog --build-path "$build_dir" --stage "$payload" \
                    --runtime-target psx-runtime
[ -f "$payload/bios/openbios.bin" ] || { echo "bundled OpenBIOS is missing" >&2; exit 1; }
[ -f "$payload/bios/OpenBIOS.LICENSE" ] || { echo "OpenBIOS license is missing" >&2; exit 1; }

mkdir -p "$payload/licenses"
[ ! -f "$fw/runtime/licenses/libchdr-NOTICES.txt" ] ||
    cp "$fw/runtime/licenses/libchdr-NOTICES.txt" "$payload/licenses/"

# --- prebuilt overlay cache + overlay toolchain ---------------------------
# The cache namespace and toolchain layout are framework-owned. The cache source
# root is the parent of the per-game directory, matching compile_overlays.py
# --out-dir and the Windows packager.
cache_src_root=${OVERLAY_CACHE_DIR:-"$root/build-linux-cache/cache"}
case "$cache_src_root" in
    *QUARANTINE*) echo "refusing quarantined overlay cache source: $cache_src_root" >&2; exit 1 ;;
esac
psx_add_overlay_cache --game-id "$game_id" \
                      --cache-src-root "$cache_src_root" \
                      --stage "$payload" \
                      --cg-tag "$cg_tag"
psx_add_overlay_toolchain --stage "$payload" \
                          --recomp-dir "$(dirname -- "$recompiler_bin")" \
                          --recomp-tools "$fw/tools" \
                          --recomp-include "$fw/runtime/include" \
                          --dl-cache "$tools_dir" \
                          --platform linux
cp "$player_toml" "$payload/game.toml"
cp "$root/packaging/release/input.ini" "$payload/input.ini"
cp "$root/packaging/release/START_HERE.txt" "$payload/START_HERE.txt"
cp "$root/packaging/linux/README.md" "$payload/APPIMAGE_README.md"
cp "$root/LICENSE" "$root/README.md" "$payload/"
[ ! -f "$root/RELEASE_NOTES.md" ] || cp "$root/RELEASE_NOTES.md" "$payload/"

# recomp-ui resolves immutable assets relative to the real ELF path.
ln -s "../share/$PAYLOAD_DIR/assets" "$appdir/usr/bin/assets"
if command -v magick >/dev/null 2>&1; then image_tool=magick
elif command -v convert >/dev/null 2>&1; then image_tool=convert
else echo "ImageMagick is required for the AppImage icon" >&2; exit 1
fi
"$image_tool" "$root/$ICON_SOURCE" -resize 240x240 -background transparent \
    -gravity center -extent 256x256 "$appdir/$DESKTOP_ID.png"
ln -s "$DESKTOP_ID.png" "$appdir/.DirIcon"

mkdir -p "$tools_dir"
fetch_tool() {
    url=$1 sha=$2 dest=$3
    if [ ! -f "$dest" ] || [ "$(sha256sum "$dest" | awk '{print $1}')" != "$sha" ]; then
        curl -fL --retry 3 "$url" -o "$dest.tmp"
        printf '%s  %s\n' "$sha" "$dest.tmp" | sha256sum -c -
        mv "$dest.tmp" "$dest"
    fi
    chmod 0755 "$dest"
}
linuxdeploy=$tools_dir/linuxdeploy-x86_64.AppImage
appimagetool=$tools_dir/appimagetool-x86_64.AppImage
appimage_runtime=$tools_dir/runtime-x86_64
fetch_tool \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
    421ca71d5c69ea97c6309276232990d43df1dcece0edfaa26bbf926ff96ed12e \
    "$linuxdeploy"
fetch_tool \
    https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage \
    a6d71e2b6cd66f8e8d16c37ad164658985e0cf5fcaa950c90a482890cb9d13e0 \
    "$appimagetool"
fetch_tool \
    https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64 \
    1cc49bcf1e2ccd593c379adb17c9f85a36d619088296504de95b1d06215aebbf \
    "$appimage_runtime"

export NO_STRIP=1
"$linuxdeploy" --appimage-extract-and-run \
    --appdir "$appdir" \
    --executable "$appdir/usr/bin/$EXE_NAME" \
    --desktop-file "$appdir/$DESKTOP_ID.desktop" \
    --icon-file "$appdir/$DESKTOP_ID.png"

find "$appdir" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +
rm -f -- "$output"
ARCH=x86_64 "$appimagetool" --appimage-extract-and-run \
    --runtime-file "$appimage_runtime" "$appdir" "$output"
chmod 0755 "$output"
sha256sum "$output"
echo "AppImage: $output"
