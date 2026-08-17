#!/usr/bin/env pwsh
param(
    [Parameter(Position = 0)] [string]$Command,
    [Parameter(Position = 1, ValueFromRemainingArguments = $true)] [string[]]$Rest
)

$RepoRoot = Split-Path -Parent $PSScriptRoot

function Show-Usage {
    @"
liqology - cargo-equivalent wrapper over CMakePresets + vcpkg

Usage:
  liqology.ps1 new <name>          Scaffold a new consumer project from this template
  liqology.ps1 add <package>       Add a vcpkg dependency to vcpkg.json (cargo add equivalent)
  liqology.ps1 build [preset]      Configure + build (default preset: release)
  liqology.ps1 run [preset] [args] Build then run the hello example
  liqology.ps1 test-sanitize       Build + run under asan-ubsan preset
  liqology.ps1 tidy                Run clang-tidy over include/
"@
}

function Assert-VcpkgRoot {
    if (-not $env:VCPKG_ROOT) {
        Write-Error @"
error: VCPKG_ROOT is not set.
  liqology needs VCPKG_ROOT pointing at a vcpkg checkout to resolve dependencies.
  Fix: git clone https://github.com/microsoft/vcpkg; `$env:VCPKG_ROOT = "`$PWD\vcpkg"; & "`$env:VCPKG_ROOT\bootstrap-vcpkg.bat"
"@
        exit 1
    }
}

function Invoke-New {
    param([string]$Name)
    if (-not $Name) { throw "usage: liqology.ps1 new <name>" }
    New-Item -ItemType Directory -Force -Path $Name | Out-Null
    foreach ($item in @('CMakeLists.txt','CMakePresets.json','vcpkg.json','vcpkg-configuration.json','.clang-tidy','cmake','vcpkg-triplets','include')) {
        Copy-Item -Recurse -Force (Join-Path $RepoRoot $item) $Name
    }
    New-Item -ItemType Directory -Force -Path (Join-Path $Name 'src') | Out-Null
    Write-Host "Scaffolded $Name from liqology template."
}

function Invoke-Add {
    param([string]$Package)
    if (-not $Package) { throw "usage: liqology.ps1 add <package>" }
    $manifestPath = Join-Path $RepoRoot 'vcpkg.json'
    $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
    $deps = @($manifest.dependencies)
    $already = $deps | Where-Object {
        if ($_ -is [string]) { $_ -eq $Package } else { $_.name -eq $Package }
    }
    if ($already) {
        Write-Host "$Package is already a dependency, no change made."
        return
    }
    $manifest.dependencies = $deps + $Package
    ($manifest | ConvertTo-Json -Depth 10) | Set-Content -Path $manifestPath -Encoding utf8
    Write-Host "Added $Package to vcpkg.json."
}

function Invoke-Build {
    param([string]$Preset = 'release')
    Assert-VcpkgRoot
    if ($Preset -eq 'wasip1-release' -and -not $env:WASI_SDK_PREFIX) {
        Write-Error @"
error: WASI_SDK_PREFIX is not set (required for the wasip1-release preset).
  Fix: download wasi-sdk from https://github.com/WebAssembly/wasi-sdk/releases and
       set `$env:WASI_SDK_PREFIX = "C:\path\to\wasi-sdk"
"@
        exit 1
    }
    cmake --preset $Preset -S $RepoRoot -B (Join-Path $RepoRoot "build/$Preset")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build (Join-Path $RepoRoot "build/$Preset")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Invoke-Run {
    param([string]$Preset = 'release', [string[]]$Args)
    Invoke-Build -Preset $Preset
    & (Join-Path $RepoRoot "build/$Preset/examples/hello/liqology_hello") @Args
}

function Invoke-TestSanitize {
    Invoke-Build -Preset 'asan-ubsan'
    & (Join-Path $RepoRoot "build/asan-ubsan/examples/hello/liqology_hello")
}

function Invoke-Tidy {
    Invoke-Build -Preset 'debug'
    clang-tidy -p (Join-Path $RepoRoot "build/debug") (Join-Path $RepoRoot "include/liqology/*.hpp")
}

if ($Rest -and $Rest.Length -gt 0) { $FirstArg = $Rest[0] } else { $FirstArg = 'release' }
$NameArg = if ($Rest -and $Rest.Length -gt 0) { $Rest[0] } else { $null }

switch ($Command) {
    'new'           { Invoke-New -Name $NameArg }
    'add'           { Invoke-Add -Package $NameArg }
    'build'         { Invoke-Build -Preset $FirstArg }
    'run'           { Invoke-Run -Preset $FirstArg -Args ($Rest[1..($Rest.Length-1)]) }
    'test-sanitize' { Invoke-TestSanitize }
    'tidy'          { Invoke-Tidy }
    default         { Show-Usage; exit 1 }
}
