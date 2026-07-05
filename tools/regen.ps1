$ErrorActionPreference = 'Stop'

$Root      = Split-Path -Parent $PSScriptRoot
$Framework = Join-Path $Root 'psxrecomp-v4'
$Tool      = Join-Path $Framework 'recompiler/build/psxrecomp-game.exe'
$Config    = Join-Path $Root 'game.toml'

if (!(Test-Path $Tool)) {
    throw "psxrecomp-game not built: $Tool"
}
if (!(Test-Path $Config)) {
    throw "game.toml not found: $Config"
}

# Config-driven invocation. The TOML describes exe, seeds, out_dir —
# see psxrecomp-v4/docs/config_schema.md.
Push-Location $Root
try {
    & $Tool --config $Config
    if ($LASTEXITCODE -ne 0) {
        throw "psxrecomp-game exited with code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
