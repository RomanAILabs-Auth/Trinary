<#
.SYNOPSIS
    Trinary + TriPy unified build orchestrator.

.DESCRIPTION
    Single-toolchain build via `zig cc` (bundled Clang + MinGW-w64 libc + lld).
    No Windows SDK dependency.

    Outputs:
      build\lib\libtrinary.a         — static archive (engine + kernels + asm)
      build\bin\trinary.exe          — native CLI
      tripy\src\tripy\_core*.pyd     — Python extension (statically linked)

.PARAMETER Config
    Release (default) or Debug.

.PARAMETER Tests
    Also build & run engine + python tests after a successful build.

.PARAMETER Clean
    Delete the build/ directory first.
#>
[CmdletBinding()]
param(
    [ValidateSet('Release','Debug')]
    [string]$Config = 'Release',
    [switch]$Tests,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$root  = $PSScriptRoot
$zig   = Join-Path $root 'tools\zig\zig.exe'
if (-not (Test-Path $zig)) { throw "zig not found: $zig (run scripts/install_zig.ps1)" }

$build = Join-Path $root 'build'
$obj   = Join-Path $build 'obj'
$lib   = Join-Path $build 'lib'
$bin   = Join-Path $build 'bin'

if ($Clean -and (Test-Path $build)) { Remove-Item $build -Recurse -Force }
foreach ($d in @($obj, $lib, $bin)) { New-Item -ItemType Directory -Path $d -Force | Out-Null }

# Common flags
$target   = 'x86_64-windows-gnu'
$include  = "-I$root\engine\include"
$warn     = @('-Wall','-Wextra','-Wpedantic','-Wno-unused-parameter','-Wno-unused-function')
$arch = @('-mavx2','-mxsave','-mxsaveopt','-mpopcnt','-mbmi2','-mfma')
if ($Config -eq 'Release') {
    $opt = @('-O3','-DNDEBUG','-ffunction-sections','-fdata-sections') + $arch
    $variant = 'release'
} else {
    $opt = @('-O0','-g','-DDEBUG') + $arch
    $variant = 'debug'
}
$std      = '-std=c11'
$defs     = @("-DTRINARY_BUILD_VARIANT=$variant")

$asmSources = @(
    'engine\asm\x86_64\braincore_u8_avx2.S',
    'engine\asm\x86_64\harding_gate_i16_avx2.S',
    'engine\asm\x86_64\lattice_flip_avx2.S'
)

$cSources = @(
    'engine\src\cpuid.c',
    'engine\src\allocator.c',
    'engine\src\dispatch.c',
    'engine\src\capi.c',
    'engine\src\rng.c',
    'engine\src\timing.c',
    'engine\src\version.c',
    'engine\src\braincore_scalar.c',
    'engine\src\harding_gate_scalar.c',
    'engine\src\lattice_flip_scalar.c',
    'engine\src\rotor_cl4_avx2.c',
    'engine\src\rotor_cl4_scalar.c',
    'engine\src\prime_sieve_scalar.c',
    'engine\compiler\lexer.c',
    'engine\compiler\sema.c',
    'engine\compiler\ir.c',
    'engine\compiler\optimizer.c',
    'engine\compiler\parser.c'
)

function Invoke-Cc {
    param([Parameter(Mandatory)][string[]]$CcArgs)
    & $script:zig cc -target $script:target @CcArgs
    if ($LASTEXITCODE -ne 0) { throw "zig cc failed: $($CcArgs -join ' ')" }
}

$objs = @()

Write-Host "[build] assembling kernels (AVX2) ..." -ForegroundColor Cyan
foreach ($s in $asmSources) {
    $src = Join-Path $root $s
    $dst = Join-Path $obj ([IO.Path]::ChangeExtension([IO.Path]::GetFileName($s),'.obj'))
    Invoke-Cc -CcArgs (@('-c', $src, '-o', $dst, $include) + $opt)
    $objs += $dst
}

Write-Host "[build] compiling C sources ..." -ForegroundColor Cyan
foreach ($s in $cSources) {
    $src = Join-Path $root $s
    $dst = Join-Path $obj ([IO.Path]::ChangeExtension([IO.Path]::GetFileName($s),'.obj'))
    Invoke-Cc -CcArgs (@('-c', $src, '-o', $dst, $std, $include) + $opt + $warn + $defs)
    $objs += $dst
}

Write-Host "[build] creating libtrinary.a ..." -ForegroundColor Cyan
$libPath = Join-Path $lib 'libtrinary.a'
if (Test-Path $libPath) { Remove-Item $libPath -Force }
& $zig ar rcs $libPath @objs
if ($LASTEXITCODE -ne 0) { throw "zig ar failed" }

Write-Host "[build] linking trinary.exe ..." -ForegroundColor Cyan
$mainSrc = Join-Path $root 'engine\src\main.c'
$exePath = Join-Path $bin 'trinary.exe'
Invoke-Cc -CcArgs (@($mainSrc, '-o', $exePath, $libPath, $std, $include, '-lbcrypt') + $opt + $warn + $defs)

Write-Host "[build] linking trinary-bench.exe ..." -ForegroundColor Cyan
$benchSrc = Join-Path $root 'benchmarks\bench_engine.c'
$benchExe = Join-Path $bin 'trinary-bench.exe'
Invoke-Cc -CcArgs (@($benchSrc, '-o', $benchExe, $libPath, $std, $include, '-lbcrypt') + $opt + $warn + $defs)

Write-Host "[build] tripy._core extension ..." -ForegroundColor Cyan
$pyInc = & python -c "import sysconfig; print(sysconfig.get_config_var('INCLUDEPY'))"
$pyLibDir = & python -c "import sysconfig, os; print(os.path.join(sysconfig.get_config_var('installed_base'), 'libs'))"
$pyVer = & python -c "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')"
$pyLib = Join-Path $pyLibDir "python$pyVer.lib"
if (-not (Test-Path $pyLib)) { throw "python lib not found: $pyLib" }

$coreSrc = Join-Path $root 'tripy\src\_core\module.c'
$pydOut  = Join-Path $root "tripy\src\tripy\_core.cp$pyVer-win_amd64.pyd"

# Build Python extension as a shared library. Use MSVC-compatible target so we
# can link against python3XX.lib (MS-style import lib).
Invoke-Cc -CcArgs (@('-shared','-o', $pydOut, $coreSrc, $libPath, "-I$pyInc", $include,
             "-L$pyLibDir", "-lpython$pyVer", '-lbcrypt') + $opt + $warn + $defs)

Write-Host "[build] OK" -ForegroundColor Green
Write-Host "  exe:  $exePath"    -ForegroundColor DarkGray
Write-Host "  lib:  $libPath"    -ForegroundColor DarkGray
Write-Host "  pyd:  $pydOut"     -ForegroundColor DarkGray

if ($Tests) {
    Write-Host "[test] running engine + python tests ..." -ForegroundColor Cyan
    & (Join-Path $root 'scripts\run_tests.ps1')
    if ($LASTEXITCODE -ne 0) { throw "tests failed" }
}
