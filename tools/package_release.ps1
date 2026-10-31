# Compatibility entrypoint for the maintained shared release builder.
# Use GNU-style shared arguments: --channel public --platform windows --jobs 2.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Framework = if ($env:PSXRECOMP_ROOT) { $env:PSXRECOMP_ROOT } else {
    Join-Path $Root 'psxrecomp-v4'
}
& (Join-Path $Framework 'tools/package_title_release.ps1') --root $Root --jobs 2 @args
exit $LASTEXITCODE
