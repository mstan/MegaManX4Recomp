#!/usr/bin/env bash
# Verify the packaged layout without opening a game window.
set -euo pipefail

appimage=${1:-}
[ -n "$appimage" ] || { echo "usage: $0 <AppImage>" >&2; exit 2; }
case "$appimage" in
    [A-Za-z]:[/\\]*) appimage=$(wslpath -u "$appimage") ;;
esac
[ -x "$appimage" ] || { echo "not executable: $appimage" >&2; exit 1; }

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
# shellcheck source=/dev/null
. "$root/packaging/release/app.conf"
expected_version=$(tr -d ' \t\r\n' < "$root/packaging/release/VERSION")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT HUP INT TERM

data_dir=$(env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run)
[ "$data_dir" = "$work/data" ] || { echo "unexpected seeded data path: $data_dir" >&2; exit 1; }

fail=0
check_file() { [ -f "$data_dir/$1" ] || { echo "MISSING file: $1" >&2; fail=1; }; }
check_dir() { [ -d "$data_dir/$1" ] || { echo "MISSING dir: $1" >&2; fail=1; }; }
for dir in saves cache mods mods/bundled assets bios; do check_dir "$dir"; done
for file in game.toml input.ini START_HERE.txt LICENSE README.md \
            bios/openbios.bin bios/OpenBIOS.LICENSE .appimage-layout-version; do
    check_file "$file"
done

got_version=$(tr -d ' \t\r\n' < "$data_dir/.appimage-layout-version")
[ "$got_version" = "$expected_version" ] ||
    { echo "version marker '$got_version' != '$expected_version'" >&2; fail=1; }
legacy_packages=$(find "$data_dir/mods" -path "$data_dir/mods/packages" -type d | wc -l)
[ "$legacy_packages" -eq 0 ] ||
    { echo "seeded legacy mods/packages catalog" >&2; fail=1; }
package_dirs=$(find "$data_dir/mods/bundled" -mindepth 1 -maxdepth 1 -type d | wc -l)
mod_count=$(find "$data_dir/mods/bundled" -mindepth 2 -maxdepth 2 -name manifest.toml | wc -l)
[ "$package_dirs" -gt 0 ] && [ "$mod_count" -eq "$package_dirs" ] ||
    { echo "seeded mod catalog has $package_dirs package dir(s) and $mod_count manifest(s)" >&2; fail=1; }

so_count=$(find "$data_dir/cache" -path '*/gcc/linux-x64/*' -name '*.so' | wc -l)
range_count=$(find "$data_dir/cache" -path '*/gcc/linux-x64/*' -name '*.ranges' | wc -l)
[ "$so_count" -gt 0 ] || { echo "seeded cache contains no Linux .so shards" >&2; fail=1; }
[ "$range_count" -ge "$so_count" ] ||
    { echo "seeded $so_count .so but only $range_count .ranges files" >&2; fail=1; }
dll_count=$(find "$data_dir/cache" -name '*.dll' | wc -l)
[ "$dll_count" -eq 0 ] || { echo "seeded cache contains Windows DLL shards" >&2; fail=1; }

# Release configuration may use relative game resource paths, but it must not
# carry the developer machine's drive letters, backslashes, py launcher, or
# MinGW paths into native Linux/Proton.
if grep -En '(^|[[:space:]"'\''])([A-Za-z]:[\\/]|\\\\)|py -3|mingw|\.exe([[:space:]"'\'']|$)' \
        "$data_dir/game.toml"; then
    echo "game.toml contains Windows-only pathing" >&2
    fail=1
fi
if grep -Eq '^[[:space:]]*disc[[:space:]]*=' "$data_dir/game.toml"; then
    echo "release game.toml preselects a developer disc path" >&2
    fail=1
fi

stray=$(find "$data_dir" \( -iname 'SCPH*.BIN' -o -iname '*.cue' -o -iname '*.bin' \
    -o -iname '*.iso' -o -iname '*.mcd' -o -name 'overlay_captures.json' \) -print)
stray=$(printf '%s\n' "$stray" | grep -v '/bios/openbios.bin$' || true)
[ -z "$stray" ] ||
    { echo "payload contains private/retail data:$stray" >&2; fail=1; }

# Reseeding must preserve player-owned files and player-generated cache shards.
printf '\n; player edit\n' >> "$data_dir/input.ini"
printf 'player-built\n' > "$data_dir/cache/.user-shard-probe"
before=$(sha256sum "$data_dir/input.ini" | awk '{print $1}')
env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run >/dev/null
after=$(sha256sum "$data_dir/input.ini" | awk '{print $1}')
[ "$before" = "$after" ] || { echo "reseed overwrote input.ini" >&2; fail=1; }
[ "$(cat "$data_dir/cache/.user-shard-probe")" = "player-built" ] ||
    { echo "reseed overwrote player cache data" >&2; fail=1; }

[ "$fail" -eq 0 ] || { echo "AppImage layout test FAILED" >&2; exit 1; }
echo "AppImage layout test passed ($expected_version, $so_count Linux overlay shard(s))"
