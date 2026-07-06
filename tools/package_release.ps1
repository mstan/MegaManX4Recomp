param(
    [string]$Version = "v0.0.1-alpha",
    [string]$BuildDir = "build-release",
    # Where your accumulated overlay cache lives (the dir compile_overlays.py
    # writes to, per game.toml overlay_autocompile_cmd --out-dir). Bundled as a
    # head start; optional. X4's cache lives at build-release/cache/SLUS-00561.
    [string]$CacheBuildDir = "build-release"
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
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

# X4 builds against its psxrecomp-v4 junction (-> the shared master framework
# worktree), NOT the master ..\psxrecomp checkout. All framework paths go
# through the junction at $Root so this game's framework pin is honored.
$RecompDir = Resolve-Path (Join-Path $Root "psxrecomp-v4\recompiler\build")
Invoke-Native { cmake --build $RecompDir --target psxrecomp-game -j $env:NUMBER_OF_PROCESSORS } "recompiler build"
& (Join-Path $RecompDir "psxrecomp-game.exe") --config (Join-Path $Root "game.toml")
if ($LASTEXITCODE -ne 0) { throw "game regen failed" }

Invoke-Native { cmake -S $Root -B $BuildPath -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSX_DEBUG_TOOLS=OFF -DPSX_LAUNCHER=ON } "cmake configure"
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
Copy-Item (Join-Path $Root "README.md") $Stage
Copy-Item (Join-Path $Root "LICENSE") $Stage
if (Test-Path (Join-Path $Root "RELEASE_NOTES.md")) {
    Copy-Item (Join-Path $Root "RELEASE_NOTES.md") $Stage
}

# Launcher assets (RML + fonts + images). The PSX_LAUNCHER build stages these
# next to the exe via a cmake POST_BUILD copy of the framework's launcher assets,
# then this repo's launcher_art/ is copied on top (X4 disc art). They live flat
# under $BuildPath (launcher.rml, fonts/, img/). A release without them shows a
# blank/broken launcher. Assert + copy.
$LauncherRml = Join-Path $BuildPath "launcher.rml"
if (-not (Test-Path $LauncherRml)) {
    throw "Launcher assets missing at $BuildPath (no launcher.rml) -- was the build configured with -DPSX_LAUNCHER=ON?"
}
Copy-Item $LauncherRml $Stage
foreach ($dir in @("fonts","img")) {
    $src = Join-Path $BuildPath $dir
    if (-not (Test-Path $src)) { throw "Launcher asset dir missing: $src" }
    Copy-Item -Recurse -Force $src (Join-Path $Stage $dir)
}
$fontCount = (Get-ChildItem (Join-Path $Stage "fonts") -Filter *.ttf -ErrorAction SilentlyContinue).Count
$imgCount  = (Get-ChildItem (Join-Path $Stage "img") -Filter *.png -ErrorAction SilentlyContinue).Count
Write-Host "Bundled launcher assets: launcher.rml + $fontCount font(s) + $imgCount image(s)"

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
disc = "mmx4/Mega Man X4.cue"
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

# Disc read speed. "1x" is authentic PlayStation timing and is the safe default:
# speeding up the emulated CD device changes how many frames pass between the
# game's internal steps, which desyncs streamed audio and wedges timing-sensitive
# Mega Man X engine loops. Fast loads instead come from turbo_loads below (which
# fast-forwards the whole machine during a load, preserving timing).
disc_speed = "1x"

# Turbo loads: while a load is in progress, run the machine at full host speed so
# loading finishes much faster, with all game timing preserved. Audio plays
# through normally. On by default. Toggleable in the launcher (Settings -> Turbo
# loads).
turbo_loads = true

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
# renderer: "software" = CPU renderer (this release's default, the validated
# path for X4). "opengl" = hardware GPU renderer, selectable in the launcher
# (Settings -> Renderer) for anyone who prefers it.
renderer = "software"
# auto_skip_fmv: skip full-motion videos (the X vs. Zero opening cinematics).
# Off by default so you see the intro. When on, a video is skipped the instant
# it starts. Toggleable in the launcher (Settings -> "Skip FMVs").
auto_skip_fmv = false
# aspect_ratio: "4:3" (native). X4 ships 4:3 only this release; widescreen is
# not offered for this title (see [widescreen] below and ISSUES.md #2).
aspect_ratio = "4:3"

# ---- Controller ---------------------------------------------------------
# X4 predates the DualShock and its pad driver REJECTS analog pads (with one
# presented, the title screen ignores Start entirely) - exactly like the real
# console. The runtime therefore presents the plain digital pad X4 expects;
# lock_mode hides the launcher's pad-mode selector because there is exactly one
# mode the game supports. deadzone: stick dead-band for stick->d-pad mapping
# (0..32767; ~12000 = 37%), adjustable in the launcher.
[controller]
default_mode = "digital"
deadzone = 12000
allow_hybrid = false
lock_mode = true

# ---- Widescreen ---------------------------------------------------------
# Widescreen is NOT offered for X4 this release (offer=false): the launcher's
# Widescreen toggle is hidden and the display aspect is clamped to native 4:3.
# full_2d stays declared for the eventual port (same pure-2D mechanism as MMX6)
# but is inert while offer=false.
[widescreen]
offer   = false
full_2d = true
"@ | Set-Content -Encoding ASCII (Join-Path $Stage "game.toml")

# Prebuilt overlay cache: native code for the game areas contributed so far.
# The cache is namespaced per backend/arch/codegen-version:
#   gcc/<arch-abi>/cg<N>/<entry8>_<crc8>.dll (+ .ranges)
# and the loader scans it by that exact path, so the subtree must be preserved.
# Ship .dll + .ranges only (skip the _patched.c intermediates and the reserved
# sljit/ namespace, which has no on-disk blobs), and ONLY the dir matching THIS
# build's codegen tag -- a stale-hash dir is dead weight the runtime never loads.
$RecompTools = Resolve-Path (Join-Path $Root "psxrecomp-v4\tools")
$RecompInc   = Resolve-Path (Join-Path $Root "psxrecomp-v4\runtime\include")
$tagScript = Join-Path $env:TEMP ("psx_cgtag_{0}.py" -f $PID)
@"
import importlib.util
s = importlib.util.spec_from_file_location('co', r'$RecompTools\compile_overlays.py')
m = importlib.util.module_from_spec(s); s.loader.exec_module(m)
inc = r'$RecompInc'
print('cg%d_%08x' % (m.codegen_ver(inc), m.codegen_hash(inc)))
"@ | Set-Content -Encoding ASCII $tagScript
$CgTag = (& python $tagScript).Trim()
Remove-Item -Force $tagScript
Write-Host "Release codegen tag: $CgTag (only this cache namespace is shipped)"
$CacheSrc = Join-Path $Root "$CacheBuildDir/cache/SLUS-00561"
if (Test-Path $CacheSrc) {
    $CacheDst = Join-Path $Stage "cache/SLUS-00561"
    $cacheFiles = Get-ChildItem $CacheSrc -Recurse -File -Include *.dll,*.ranges |
        Where-Object { $_.FullName -notmatch '[\\/]sljit[\\/]' -and $_.FullName -match "[\\/]$CgTag[\\/]" }
    foreach ($f in $cacheFiles) {
        $rel  = $f.FullName.Substring($CacheSrc.Length).TrimStart('\','/')
        $dest = Join-Path $CacheDst $rel
        New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
        Copy-Item $f.FullName $dest
    }
    # Only files under THIS build's codegen tag are ABI-compatible; if none match
    # (e.g. the tag moved after a framework change and no area has been re-JITed
    # under the new tag yet), ship cache-less rather than aborting -- the bundled
    # tcc toolchain JIT-compiles overlays at runtime on first visit.
    if (Test-Path $CacheDst) {
        $dllCount = (Get-ChildItem $CacheDst -Recurse -Filter *.dll).Count
        Write-Host "Bundled overlay cache: $dllCount native overlay DLL(s)"
    } else {
        Write-Warning "No overlay cache matching codegen tag $CgTag under $CacheSrc - releasing without a head-start cache (overlays JIT-compile at runtime via the bundled tcc toolchain)"
    }
} else {
    Write-Warning "No overlay cache found at $CacheSrc - releasing without bundled cache"
}

# ---- Self-contained overlay toolchain (tcc tier) -------------------------
# A player box has no gcc AND no Python, so overlay_backend=auto resolves to tcc:
# the runtime fills overlay gaps the shipped gcc cache misses by spawning this
# bundled, fully self-contained toolchain. The runtime constructs the command
# from <exe>/overlay_toolchain/ (see main.cpp): embedded Python + TinyCC + the
# recompiler + compile_overlays.py + the runtime headers. Every exe here must be
# self-contained (embedded python + prebuilt tcc are; the recompiler needs its
# mingw runtime DLLs bundled beside it).
$Toolchain = Join-Path $Stage "overlay_toolchain"
New-Item -ItemType Directory -Force $Toolchain | Out-Null
$DlCache = Join-Path $Root "tools/_toolchain_cache"
New-Item -ItemType Directory -Force $DlCache | Out-Null

# Embedded Python (fixed version; downloaded once + cached)
$PyVer = "3.13.1"
$PyZip = Join-Path $DlCache "python-$PyVer-embed-amd64.zip"
if (-not (Test-Path $PyZip)) {
    Invoke-WebRequest -Uri "https://www.python.org/ftp/python/$PyVer/python-$PyVer-embed-amd64.zip" -OutFile $PyZip
}
Expand-Archive -Path $PyZip -DestinationPath (Join-Path $Toolchain "python") -Force

# TinyCC prebuilt win64 (fixed version; downloaded once + cached). The zip has a
# top-level tcc/ dir (tcc.exe + libtcc.dll + include/ + lib/) — ship it whole.
$TccZip = Join-Path $DlCache "tcc-0.9.27-win64-bin.zip"
if (-not (Test-Path $TccZip)) {
    Invoke-WebRequest -Uri "https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip" -OutFile $TccZip
}
$TccTmp = Join-Path $DlCache "tcc_extract"
if (Test-Path $TccTmp) { Remove-Item -Recurse -Force $TccTmp }
Expand-Archive -Path $TccZip -DestinationPath $TccTmp -Force
Copy-Item -Recurse -Force (Join-Path $TccTmp "tcc") (Join-Path $Toolchain "tcc")

# Recompiler (built above) + its mingw runtime DLLs (NOT statically linked) +
# compile_overlays.py + the runtime headers.
Copy-Item (Join-Path $RecompDir "psxrecomp-game.exe") $Toolchain
foreach ($d in @("libgcc_s_seh-1.dll","libstdc++-6.dll","libwinpthread-1.dll")) {
    Copy-Item (Join-Path $MingwBin $d) $Toolchain
}
Copy-Item (Resolve-Path (Join-Path $Root "psxrecomp-v4\tools\compile_overlays.py")) $Toolchain
$ToolInc = Join-Path $Toolchain "include"
New-Item -ItemType Directory -Force $ToolInc | Out-Null
Copy-Item (Join-Path (Resolve-Path (Join-Path $Root "psxrecomp-v4\runtime\include")) "*.h") $ToolInc
$tcMB = "{0:N0}" -f ((Get-ChildItem $Toolchain -Recurse -File | Measure-Object Length -Sum).Sum / 1MB)
Write-Host "Bundled overlay toolchain (embedded python + tcc + recompiler): ~$tcMB MB"

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
deadzone = 12000

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

This package does not include the Mega Man X4 disc, the PlayStation BIOS, save
data, or any game assets - you supply those from your own collection, and
MegaManX4Recomp asks for them one at a time (each dialog says which one it
wants). The executable and the cache folder contain statically recompiled
(machine-translated) builds of the game's code, the same distribution model
used by other static recompilation projects such as N64: Recompiled.

First launch:
1. Run MegaManX4Recomp.exe. A launcher window opens.
2. In the launcher, set your PlayStation BIOS: select your legally obtained
   SCPH1001.BIN (a 512 KB file dumped from your own console).
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

Options such as turbo loads, FMV skip, and disc speed can be changed in the
launcher Settings or in game.toml ([runtime]/[video]) with any text editor.

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
