<#
.SYNOPSIS
    Run engine + python tests against an existing build.
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$zig  = Join-Path $root 'tools\zig\zig.exe'
if (-not (Test-Path $zig)) { throw "zig not found - run build.ps1 first" }

$build  = Join-Path $root 'build'
$bin    = Join-Path $build 'bin'
$lib    = Join-Path $build 'lib\libtrinary.a'
if (-not (Test-Path $lib)) { throw "missing $lib - run build.ps1 first" }

$testExe = Join-Path $bin 'trinary_tests.exe'
$testSrc = Join-Path $root 'engine\tests\test_kernels.c'

& $zig cc -target x86_64-windows-gnu -O2 -std=c11 `
    "-I$root\engine\include" "-I$root\engine\compiler" $testSrc $lib -lbcrypt -o $testExe
if ($LASTEXITCODE -ne 0) { throw "engine test compile failed" }

Write-Host "[test] engine kernels + benchmarks/perf_gate self-tests ..." -ForegroundColor Cyan
& $testExe
if ($LASTEXITCODE -ne 0) { throw "engine kernel tests FAILED" }

Write-Host "[test] python (tripy + perf_gate) ..." -ForegroundColor Cyan
Push-Location $root
try {
    $env:PYTHONPATH = "$root\tripy\src;$env:PYTHONPATH"
    & python -m pytest tripy\tests benchmarks\tests -q
    if ($LASTEXITCODE -ne 0) { throw "pytest failed" }
} finally {
    Pop-Location
}

Write-Host "[test] OK" -ForegroundColor Green
