param(
    [string]$Version = "v0.0.5-alpha",
    [string]$BuildDir = "build-release",
    # Where your accumulated overlay cache lives (the dir compile_overlays.py
    # writes to, per game.toml overlay_autocompile_cmd --out-dir). Bundled as a
    # head start; optional. X4's cache lives at build-release/cache/SLUS-00561.
    [string]$CacheBuildDir = "build-release",
    # Framework checkout to build against. Defaults to the psxrecomp-v4 junction
    # (-> the shared master checkout). Point this at a specific worktree to build
    # against exactly the pinned commit when the main checkout is on another
    # branch (e.g. -FrameworkRoot F:/Projects/psxrecomp/_wt-fw-master).
    [string]$FrameworkRoot = ""
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
if ([string]::IsNullOrWhiteSpace($FrameworkRoot)) {
    $FrameworkRoot = Join-Path $Root "psxrecomp-v4"
}
$FrameworkRoot = (Resolve-Path $FrameworkRoot).Path
Write-Host "Framework root: $FrameworkRoot"
$BuildPath = Join-Path $Root $BuildDir
$StageRoot = Join-Path $Root "release-stage"
$Stage = Join-Path $StageRoot "MegaManX4Recomp-windows-x64"
$ZipPath = Join-Path $Root ("MegaManX4Recomp-{0}-windows-x64.zip" -f $Version)
$MingwBin = "C:\msys64\mingw64\bin"

$env:PATH = "$MingwBin;$env:PATH"

# Regenerate the game's C BEFORE building. The runtime build below just compiles
# generated/*.c, so a stale generated/ would ship the wrong code.
# cmake writes benign warnings (e.g. freetype's cmake_minimum_required
# deprecation) to STDERR. Under $ErrorActionPreference='Stop', PowerShell 5.1
# promotes a native command's stderr write to a TERMINATING error, aborting the
# release for a non-error. Run the native cmake invocations with the preference
# relaxed and gate on the real signal -- $LASTEXITCODE -- instead.
function Invoke-Native {
    param([scriptblock]$Cmd, [string]$What)
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $Cmd 2>&1 | Out-Host
    $code = $LASTEXITCODE
    $ErrorActionPreference = $old
    if ($code -ne 0) { throw "$What failed (exit $code)" }
}

function Get-TomlScalar {
    param(
        [Parameter(Mandatory)][string]$GameToml,
        [Parameter(Mandatory)][string]$Table,
        [Parameter(Mandatory)][string]$Key
    )
    $section = ""
    foreach ($raw in (Get-Content -LiteralPath $GameToml)) {
        $line = $raw.Trim()
        if (-not $line -or $line.StartsWith("#")) { continue }
        if ($line -match '^\[\[?([^\]]+)\]\]?$') { $section = $Matches[1].Trim(); continue }
        if ($section -ne $Table) { continue }
        if ($line -match ('^' + [regex]::Escape($Key) + '\s*=\s*(.+?)\s*(?:#.*)?$')) {
            return $Matches[1].Trim().Trim('"').Trim("'")
        }
    }
    return $null
}
# X4 builds against its psxrecomp-v4 junction (-> the shared master framework
# worktree), NOT the master ..\psxrecomp checkout. All framework paths go
# through the junction at $Root so this game's framework pin is honored.
$RecompDir = Resolve-Path (Join-Path $FrameworkRoot "recompiler\build")
Invoke-Native { cmake --build $RecompDir --target psxrecomp-game -j $env:NUMBER_OF_PROCESSORS } "recompiler build"
& (Join-Path $RecompDir "psxrecomp-game.exe") --config (Join-Path $Root "game.toml")
if ($LASTEXITCODE -ne 0) { throw "game regen failed" }

Invoke-Native { cmake -S $Root -B $BuildPath -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSX_DEBUG_TOOLS=OFF -DPSXRECOMP_ROOT="$FrameworkRoot" } "cmake configure"
Invoke-Native { cmake --build $BuildPath -j $env:NUMBER_OF_PROCESSORS } "cmake build"

if (Test-Path $StageRoot) {
    Remove-Item -Recurse -Force $StageRoot
}
New-Item -ItemType Directory -Force $Stage | Out-Null
New-Item -ItemType Directory -Force (Join-Path $Stage "saves") | Out-Null

# The CMake OUTPUT_NAME may already be MegaManX4Recomp.exe; accept the pre-rename
# per-game name (mmx4-runtime.exe) and the generic psx-runtime.exe too.
$DevExe = Join-Path $BuildPath "MegaManX4Recomp.exe"
if (-not (Test-Path $DevExe)) { $DevExe = Join-Path $BuildPath "mmx4-runtime.exe" }
if (-not (Test-Path $DevExe)) { $DevExe = Join-Path $BuildPath "psx-runtime.exe" }
Copy-Item $DevExe (Join-Path $Stage "MegaManX4Recomp.exe")
$BiosSrc = Join-Path $BuildPath "bios"
if (-not (Test-Path (Join-Path $BiosSrc "openbios.bin"))) {
    throw "Bundled OpenBIOS image missing at $BiosSrc"
}
Copy-Item -Recurse -Force $BiosSrc (Join-Path $Stage "bios")
Copy-Item (Join-Path $Root "README.md") $Stage
Copy-Item (Join-Path $Root "LICENSE") $Stage
if (Test-Path (Join-Path $Root "RELEASE_NOTES.md")) {
    Copy-Item (Join-Path $Root "RELEASE_NOTES.md") $Stage
}

# Launcher assets: this build ships the shared recomp-ui Dear ImGui launcher
# (RECOMP_LAUNCHER; see main.cpp + recomp-ui/recomp_ui.cmake), which loads from
# <exe>/assets/ (fonts + img TGAs, including this repo's boxart baked in by
# recomp_target_launcher_ui's POST_BUILD).
$AssetsSrc = Join-Path $BuildPath "assets"
if (-not (Test-Path (Join-Path $AssetsSrc "img"))) {
    throw "recomp-ui launcher assets missing at $AssetsSrc -- was the recomp-ui launcher built (recomp-ui junction present)?"
}
Copy-Item -Recurse -Force $AssetsSrc (Join-Path $Stage "assets")
$fontCount = (Get-ChildItem (Join-Path $Stage "assets/fonts") -Filter *.ttf -ErrorAction SilentlyContinue).Count
$imgCount  = (Get-ChildItem (Join-Path $Stage "assets/img")   -Filter *.tga -ErrorAction SilentlyContinue).Count
Write-Host "Bundled recomp-ui launcher assets: $fontCount font(s) + $imgCount image(s)"

# Built-in mod catalog staged by the runtime target's POST_BUILD command into
# <build>/mods/bundled (psxrecomp_add_runtime_target's PRELOADED_MODS_DIR
# stages the framework's mods/builtin/packages and this repo's
# mods/preloaded/packages there together).
#
# Routed through the framework's shared Add-ModCatalog. What used to be here
# was four literal
#
#     mods/packages/mmx4.<name>/1.0.0/manifest.toml
#
# paths, each asserted with Test-Path, and both halves of that were wrong:
#
#   * mods/packages is the PRE-SPLIT layout. Framework 4cc04be3 moved staged
#     build output to mods/bundled and nothing in this repo followed, so at
#     framework master the first Test-Path threw and this title could not
#     package at all (bead beads-eio.3.101);
#   * the paths pin the VERSION too, so bumping a package from 1.0.0 to 1.1.0
#     would have failed the release for a reason unrelated to the release, and
#     the list names only this repo's four packages -- it says nothing about
#     whether the framework's shared psx.* packages shipped.
#
# Add-ModCatalog asserts the invariant instead: every package the SOURCES
# define -- this repo's mods/preloaded/packages and the framework's
# mods/builtin/packages -- must survive into the staged catalog, at whatever
# version the sources carry. It also strips the two things under mods/ that
# belong to this machine (installed/ and state.toml), which is what the
# hand-written state.toml removal below it was doing by hand.
. (Join-Path $FrameworkRoot "tools\release_overlay_stage.ps1")
Add-ModCatalog -BuildPath $BuildPath -Stage $Stage `
               -GameModSource (Join-Path $Root "mods\preloaded") `
               -FrameworkModSource (Join-Path $FrameworkRoot "mods\builtin") | Out-Null

# Player-facing game.toml: same effective runtime settings as the dev config,
# minus dev-only sections ([recompiler] inputs beyond the required block, the
# gcc overlay-autocompile command, and the [audit] block). overlay_backend is
# left at the default "auto": with no gcc toolchain on a player box it resolves
# to tcc, which fills overlay gaps via the bundled overlay_toolchain/ (no system
# python or gcc needed). Players can edit [runtime]/[video] post-install.
@"
[game]
name = "Mega Man X4"
id = "SLUS-00561"
exe = "mmx4/SLUS_005.61"
load_address = "0x80010000"
entry_pc = "0x800DAE8C"
text_size = "0x0011F800"
stack_base = "0x801FFFF0"

# Required block; used only by the developer recompiler tool, not at runtime.
[recompiler]
seeds = "seeds/ghidra_funcs.txt"
out_dir = "generated"

# ---- Player-adjustable options ------------------------------------------
# Edit, save, and restart MegaManX4Recomp.exe to apply.
[runtime]
window_title = "Mega Man X4 Recompiled"
memcard_dir = "saves"

# Authentic loading baseline. The built-in Fast Loading mod owns the mutually
# exclusive host-pacing and experimental guest-visible CD-speed choices.
disc_speed = "1x"
turbo_loads = false
offer_turbo_loads = false

# Overlay cache: keeps converted native code for game areas in the cache folder,
# and records newly visited areas into overlay_captures.json so your own cache
# grows as you play. Keep that file private - it contains game code from your
# disc (see README).
overlay_cache = true

# HLE-accelerated boot (the validated configuration for X4 this release): kernel
# services are served host-side and the BIOS shell is skipped, booting straight
# into the game; everything else still runs the real recompiled BIOS. Set false
# to boot the authentic full BIOS sequence instead (unvalidated for X4).
bios_hle = true

# Host audio cushion. X4 opts into a shorter buffer than the framework's
# conservative cross-game default to reduce audible input-to-sound delay.
[audio]
buffer_ms = 60

# ---- Visual quality -----------------------------------------------------
[video]
# supersampling: render at this multiple of native resolution and downsample,
# for higher detail and anti-aliased edges. 1 = native PSX look, 2 = recommended,
# 3-4 = sharper (needs a faster CPU to hold full speed).
supersampling = 2
# antialiasing: smooth (linear) scaling to the window. false = sharp pixels.
antialiasing  = true
# texture_filtering: "nearest" = native PSX look; "bilinear" = smooths textures.
texture_filtering = "nearest"
# renderer: "opengl" = hardware GPU renderer (this release's default, full-rate
# presentation). "software" = CPU renderer, selectable in the launcher
# (Settings -> Renderer) for anyone who prefers it.
renderer = "opengl"
# auto_skip_fmv: skip full-motion videos (the X vs. Zero opening cinematics).
# Off by default so you see the intro. When on, a video is skipped the instant
# it starts. Toggleable in the launcher (Settings -> "Skip FMVs").
auto_skip_fmv = false
# X4 owns presentation-only interpolation through its built-in mod. Hide the
# duplicate generic Settings row and ignore stale values from older builds.
offer_frame_interpolation = false
# aspect_ratio: "4:3" (native). Enable the default-off Widescreen mod to opt
# into true 16:9 (see [widescreen] below).
aspect_ratio = "4:3"

# ---- Controller ---------------------------------------------------------
# X4 predates the DualShock and its pad driver REJECTS analog pads (with one
# presented, the title screen ignores Start entirely) - exactly like the real
# console. The runtime therefore presents the plain digital pad X4 expects;
# lock_mode hides the launcher's pad-mode selector because there is exactly one
# mode the game supports. deadzone: stick dead-band for stick->d-pad mapping
# (0..32767; 6553 = 20%), adjustable in the launcher.
[controller]
default_mode = "digital"
deadzone = 6553
allow_hybrid = false
lock_mode = true

# ---- Widescreen (EXPERIMENTAL) ------------------------------------------
# X4 offers an experimental default-off 16:9 mod. The exact validated hook
# config is spliced from the dev game.toml below so the shipped config can never
# drift from what was built and tested. All hooks are identity at 4:3.
"@ | Set-Content -Encoding ASCII (Join-Path $Stage "game.toml")

# Splice the real, validated [widescreen]* sections (offer=false + bg2d/cull/HUD
# hooks) straight from the dev game.toml -- single source of truth, no drift.
$realToml = Get-Content (Join-Path $Root "game.toml") -Raw
$wsStart  = $realToml.IndexOf("[widescreen]")
$wsEnd    = $realToml.IndexOf("[controller]", $wsStart)
if ($wsStart -lt 0 -or $wsEnd -lt 0) { throw "Could not locate [widescreen]..[controller] in game.toml to splice" }
$wsBlock  = $realToml.Substring($wsStart, $wsEnd - $wsStart).TrimEnd()
Add-Content -Encoding ASCII (Join-Path $Stage "game.toml") $wsBlock
if (-not (Select-String -Path (Join-Path $Stage "game.toml") -Pattern '^offer\s*=\s*false' -Quiet)) {
    throw "Shipped game.toml is missing 'offer = false' after widescreen splice"
}

# Prebuilt overlay cache + self-contained overlay toolchain, both staged by the
# shared framework implementation. The cache-required decision is read from the
# STAGED game.toml, since that file is folded into the tag and is the contract
# the released executable actually loads.
$RecompTools = (Resolve-Path (Join-Path $FrameworkRoot "tools")).Path
$RecompInc   = (Resolve-Path (Join-Path $FrameworkRoot "runtime\include")).Path
$StagedGameToml = Join-Path $Stage "game.toml"
$CacheGameId = Get-TomlScalar -GameToml $StagedGameToml -Table "game" -Key "id"
if (-not $CacheGameId) { throw "Could not read [game] id from $StagedGameToml" }

$CacheSrcRoot = if ([System.IO.Path]::IsPathRooted($CacheBuildDir)) {
    $CacheBuildDir
} else {
    Join-Path $Root $CacheBuildDir
}
foreach ($p in @($CacheSrcRoot, (Resolve-Path -LiteralPath $CacheSrcRoot -ErrorAction SilentlyContinue).Path)) {
    if ($p -and $p -match 'QUARANTINE') { throw "Refusing quarantined overlay cache source: $p" }
}
$CacheSrcRoot = Join-Path $CacheSrcRoot "cache"

$CgTag = Get-OverlayCgTag -RecompTools $RecompTools -RecompInc $RecompInc `
                          -GameExe (Join-Path $RecompDir "psxrecomp-game.exe") `
                          -GameToml $StagedGameToml `
                          -BuildPath $BuildPath -RuntimeTarget "psx-runtime"
Write-Host "Release codegen tag: $CgTag (only this cache namespace is shipped)"

$OverlayCacheDeclared =
    ((Get-TomlScalar -GameToml $StagedGameToml -Table "runtime" -Key "overlay_cache") -eq "true")
if ($OverlayCacheDeclared) {
    Write-Host "Staged game.toml declares overlay_cache = true; a shard cache is required"
    Add-OverlayCache -GameId $CacheGameId -CacheSrcRoot $CacheSrcRoot `
                     -Stage $Stage -CgTag $CgTag | Out-Null
} else {
    Write-Host "Staged game.toml does not declare overlay_cache; staging no shard cache"
}
Add-OverlayToolchain -Stage $Stage -RecompDir $RecompDir -RecompTools $RecompTools `
                     -RecompInc $RecompInc -MingwBin $MingwBin `
                     -DlCache (Join-Path $Root "tools\_toolchain_cache") | Out-Null
# The Release build is statically linked (PSX_STATIC_RUNTIME defaults ON for
# MinGW Release), so the exe imports ONLY Windows system DLLs -- nothing to
# bundle. Assert self-containment rather than trust it (mismatched side-by-side
# DLLs were the cause of the 0xc000007b launch crash on other projects).
$objdump = Join-Path $MingwBin "objdump.exe"
$imports = & $objdump -p (Join-Path $Stage "MegaManX4Recomp.exe") |
    Select-String "DLL Name: (.+)" | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }
$systemDlls = @("kernel32.dll","user32.dll","gdi32.dll","shell32.dll","msvcrt.dll",
                "advapi32.dll","ws2_32.dll","comdlg32.dll","dbghelp.dll","ole32.dll",
                "oleaut32.dll","winmm.dll","imm32.dll","version.dll","setupapi.dll",
                "dinput8.dll","rpcrt4.dll","hid.dll","cfgmgr32.dll","opengl32.dll")
$nonSystem = $imports | Where-Object { $systemDlls -notcontains $_.ToLower() }
if ($nonSystem) {
    throw "Release exe is NOT self-contained -- imports non-system DLL(s): $($nonSystem -join ', ')"
}
Write-Host "Verified self-contained: imports only system DLLs ($($imports.Count) total)"

@"
; PSXRecomp input mapping. PSX buttons are active when any listed source is pressed.
; Sources use SDL/Xbox names: a,b,x,y,back,start,leftshoulder,rightshoulder,
; lefttrigger,righttrigger,dpup,dpdown,dpleft,dpright,leftx-/leftx+/lefty-/lefty+.

[controller]
enabled = true
device = 0
deadzone = 6553

[mapping]
up = dpup,lefty-
down = dpdown,lefty+
left = dpleft,leftx-
right = dpright,leftx+
cross = a
circle = b
square = x
triangle = y
l1 = leftshoulder
r1 = rightshoulder
l2 = lefttrigger
r2 = righttrigger
start = start
select = back
"@ | Set-Content -Encoding ASCII (Join-Path $Stage "input.ini")

@"
MegaManX4Recomp $Version

Mega Man X4 boots and plays - the intro cinematics decode and play, the title
screen and menus respond, the attract demos run, and you can start a game -
with working (digital) controller input and no known crashes on the covered
path. This is an early first release cut days after first boot: it has NOT
been verified deep into stages, and an unvisited area may halt the program
with an "unknown dispatch" report (that is by design - please report where
you were; see ISSUES.md #1).

This package does not include the Mega Man X4 disc, retail PlayStation BIOS,
save data, or any game assets. It includes the redistributable MIT-licensed
OpenBIOS, so you only need to select your own game disc. You may optionally
select your own legally dumped retail BIOS instead. The executable and the
cache folder contain statically recompiled
(machine-translated) builds of the game's code, the same distribution model
used by other static recompilation projects such as N64: Recompiled.

First launch:
1. Run MegaManX4Recomp.exe. A launcher window opens.
2. The included OpenBIOS is selected automatically. You may instead select a
   legally obtained SCPH1001.BIN dumped from your own console.
3. Set the game disc: select your legally obtained Mega Man X4 (USA,
   SLUS-00561) disc image.
4. Adjust any options you like (renderer, supersampling, screen look,
   controller), then press Launch. Your choices are remembered next time.

Disc image formats:
- .cue + .bin (preferred - pick the .cue)
- .bin
Do NOT convert to a 2048-byte "cooked" .iso - it discards the XA sectors MMX4
streams its FMV/audio from.

The selected BIOS path is saved in bios.cfg and the selected disc path is saved
in disc.cfg next to the executable. Delete those files to pick different files.

Fast Loading, Damage Multiplier, Widescreen, and Interpolated Frames are owned
by the launcher's Mods page. Fast Loading defaults off; Damage Multiplier
defaults to 1 for normal damage. Other options such as FMV skip can be changed
in launcher Settings or game.toml with any text editor.

The cache folder contains pre-converted native code for game areas covered so
far; those run at full speed from your first visit. As you play, newly visited
areas are recorded into overlay_captures.json and your local cache grows
automatically. Do NOT post overlay_captures.json publicly - it contains
snapshots of the game's own code read from your disc. See README.md for details.

Keyboard and Xbox-style controller defaults are documented in README.md.
Controller mappings are configurable in input.ini; keyboard bindings in
keybinds.ini (also live-rebindable in the launcher's Controls page).

Memory cards are stored in the saves directory as standard PS1 .mcd images.
In-game save/load has not yet been verified end-to-end in this build (see
ISSUES.md #3).
"@ | Set-Content -Encoding ASCII (Join-Path $Stage "START_HERE.txt")

if (Test-Path $ZipPath) {
    Remove-Item -Force $ZipPath
}
Compress-Archive -Path (Join-Path $Stage "*") -DestinationPath $ZipPath -Force

Write-Host "Wrote $ZipPath"
