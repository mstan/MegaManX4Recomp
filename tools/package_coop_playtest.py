"""Stage an existing Windows production build through the shared release gates.

Uses a new output directory, never deletes an existing package. The runtime
may live outside its CMake build directory (CMAKE_RUNTIME_OUTPUT_DIRECTORY).
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True, cwd=ROOT)


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--binary-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--mingw-bin", type=Path, default=Path("C:/msys64/mingw64/bin"))
    args = parser.parse_args()
    build, binary, output = (path.resolve() for path in
                             (args.build_dir, args.binary_dir, args.output_dir))
    cache = (build / "CMakeCache.txt").read_text()
    for setting in ("CMAKE_BUILD_TYPE:STRING=Release", "PSX_DEBUG_TOOLS:BOOL=OFF",
                    "PSX_STATIC_RUNTIME:BOOL=ON", "PSX_EXECUTION_PROFILE:STRING=ENHANCED"):
        if setting not in cache.splitlines():
            raise RuntimeError(f"Production build requires {setting}")
    version = (ROOT / "packaging/release/VERSION").read_text().strip().removeprefix("v")
    stamp = (binary / "psx_game_version.txt").read_text().strip().removeprefix("v")
    if stamp != version:
        raise RuntimeError(f"Binary version {stamp} differs from release {version}")
    if output.exists():
        raise RuntimeError(f"Use a new output directory: {output}")
    if not output.is_relative_to(ROOT):
        raise RuntimeError("Package output must be inside this worktree")
    framework = ROOT / "psxrecomp-v4"
    gate = framework / "tools/release_stage.py"
    stage = output / "MegaManX4Recomp-windows-x64"
    stage.mkdir(parents=True)
    executable = stage / "MegaManX4Recomp.exe"
    shutil.copy2(binary / executable.name, executable)
    shutil.copytree(binary / "assets", stage / "assets")
    (stage / "bios").mkdir()
    for name in ("openbios.bin", "OpenBIOS.LICENSE"):
        shutil.copy2(binary / "bios" / name, stage / "bios" / name)
    for name in ("game.toml", "input.ini", "START_HERE.txt"):
        shutil.copy2(ROOT / "packaging/release" / name, stage / name)
    for name in ("README.md", "LICENSE", "RELEASE_NOTES.md"):
        shutil.copy2(ROOT / name, stage / name)
    shutil.copy2(binary / "psx_game_version.txt", stage / "psx_game_version.txt")
    run(sys.executable, gate, "stage-mods", "--build-path", binary,
        "--stage", stage, "--catalog-manifest", build / "psx_mod_catalog_psx-runtime.txt")
    run(sys.executable, gate, "stage-toolchain", "--stage", stage,
        "--recomp-dir", framework / "recompiler/build", "--recomp-tools", framework / "tools",
        "--recomp-include", framework / "runtime/include", "--mingw-bin", args.mingw_bin,
        "--platform", "win", "--dl-cache", ROOT / "tools/_toolchain_cache")
    run(sys.executable, gate, "stage-execution", "--binary", executable,
        "--manifest", binary / "MegaManX4Recomp.execution.json")
    imports = subprocess.check_output([str(args.mingw_bin / "objdump.exe"), "-p", str(executable)],
                                      text=True)
    dlls = re.findall(r"DLL Name:\s*(\S+)", imports)
    system = set("kernel32 user32 gdi32 shell32 msvcrt advapi32 ws2_32 comdlg32 dbghelp ole32 "
                 "oleaut32 winmm imm32 version setupapi dinput8 rpcrt4 hid cfgmgr32 opengl32 "
                 "winhttp dwmapi bcrypt shlwapi crypt32 ntdll uxtheme d2d1 dwrite iphlpapi".split())
    non_system = [dll for dll in dlls if dll.lower().removesuffix(".dll") not in system
                  and not dll.lower().startswith("api-ms-win-")]
    if non_system:
        raise RuntimeError(f"Runtime imports non-system DLLs: {non_system}")
    # No captured game code/cache or machine-specific saves/settings are shipped.
    # Original-disc inventory found no separate X4 executable overlay; resident
    # code is statically compiled. Eligible uncovered code has the shared toolchain.
    prohibited = {"saves", "cache", "installed", "state.toml", "settings.toml",
                  "disc.cfg", "bios.cfg", "overlay_captures.json"}
    bad = [str(path.relative_to(stage)) for path in stage.rglob("*")
           if path.name in prohibited or path.suffix.lower() in {".mcd", ".cue", ".gpr"}]
    if bad:
        raise RuntimeError(f"Private runtime data in package: {bad}")
    archive = output / f"MegaManX4Recomp-v{version}-windows-x64.zip"
    run(sys.executable, framework / "tools/create_release_zip.py",
        "--source", stage, "--output", archive)
    checksum = f"{sha256(archive)}  {archive.name}\n"
    (output / "SHA256SUMS.txt").write_text(checksum, encoding="ascii")
    report = dict(version=version, executable_sha256=sha256(executable),
                  zip_sha256=sha256(archive), system_imports=dlls,
                  files=len(list(stage.rglob("*"))), package=str(archive))
    (output / "package-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
