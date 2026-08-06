#!/usr/bin/env bash
# Rebuild the release's native Linux overlay cache from a private capture file.
# Capture data is intentionally not packaged; only the resulting .so/.ranges
# artifacts are consumed by package_appimage.sh.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
captures=${OVERLAY_CAPTURES:-"$root/build-master/overlay_captures.json"}
out_dir=${OVERLAY_CACHE_DIR:-"$root/build-linux-cache/cache"}
cores=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
jobs=${BUILD_JOBS:-$(( cores > 4 ? cores - 2 : 2 ))}
force=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        --captures) captures=$2; shift 2 ;;
        --out) out_dir=$2; shift 2 ;;
        --jobs) jobs=$2; shift 2 ;;
        --force) force=1; shift ;;
        -h|--help)
            echo "usage: $0 [--captures FILE] [--out CACHE_DIR] [--jobs N] [--force]"
            exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

to_unix_path() {
    case "$1" in
        [A-Za-z]:[/\\]*)
            command -v wslpath >/dev/null 2>&1 ||
                { echo "cannot translate Windows path '$1'" >&2; exit 2; }
            wslpath -u "$1" ;;
        *) printf '%s\n' "$1" ;;
    esac
}
captures=$(to_unix_path "$captures")
out_dir=$(to_unix_path "$out_dir")
[ -f "$captures" ] || { echo "overlay captures not found: $captures" >&2; exit 1; }

# compile_overlays runs the recompiler with cwd = dirname(game.toml). The
# player config lives under packaging/release, where framework auto-discovery
# cannot see psxrecomp-v4/. Use an identical temporary copy at the project root;
# content (and therefore the cache config hash) remains exactly the same.
release_toml=$root/.appimage-release-game.$$.toml
cp "$root/packaging/release/game.toml" "$release_toml"
trap 'rm -f -- "$release_toml"' EXIT HUP INT TERM

# shellcheck source=/dev/null
. "$root/packaging/release/app.conf"
fw=$root/$FRAMEWORK_DIR
build=${PSXRECOMP_BIOS_BUILD:-recompiler/build-linux}
generator=Ninja
command -v ninja >/dev/null 2>&1 || generator="Unix Makefiles"

cmake -S "$fw/recompiler" -B "$fw/$build" -G "$generator" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$fw/$build" --target psxrecomp-game -j "$jobs"
recompiler=$fw/$build/psxrecomp-game

args=(
    --captures "$captures"
    --game-toml "$release_toml"
    --recompiler "$recompiler"
    --runtime-include "$fw/runtime/include"
    --out-dir "$out_dir"
    --compiler gcc
    --gcc "$(command -v gcc)"
    --cps
    --jobs "$jobs"
)
[ "$force" = 0 ] || args+=(--force)
python3 "$fw/tools/compile_overlays.py" "${args[@]}"

game_id=$(sed -n 's/^[[:space:]]*id[[:space:]]*=[[:space:]]*"\([^"]*\)".*/\1/p' \
    "$root/packaging/release/game.toml" | head -1)
shard_root=$out_dir/$game_id/gcc/linux-x64
so_count=$(find "$shard_root" -name '*.so' 2>/dev/null | wc -l)
range_count=$(find "$shard_root" -name '*.ranges' 2>/dev/null | wc -l)
[ "$so_count" -gt 0 ] || { echo "overlay build produced no Linux .so shards" >&2; exit 1; }
[ "$range_count" -ge "$so_count" ] ||
    { echo "overlay build produced $so_count .so but only $range_count .ranges files" >&2; exit 1; }
echo "Linux overlay cache ready: $so_count .so shard(s), $range_count range manifest(s)"
