<#
Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
Contact: daniel@romanailabs.com, romanailabs@gmail.com
Website: romanailabs.com
#>
[CmdletBinding()]
param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$binDir = Join-Path $env:LOCALAPPDATA "RomanAILabs\Trinary\bin"
$targetExe = Join-Path $binDir "trinary.exe"
$sourceExe = Join-Path $root "build\bin\trinary.exe"

if (-not $SkipBuild) {
    & (Join-Path $root "build.ps1") -Config $Config
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
}

if (-not (Test-Path $sourceExe)) {
    throw "missing binary: $sourceExe (run build first)"
}

New-Item -ItemType Directory -Force -Path $binDir | Out-Null
Copy-Item -Force $sourceExe $targetExe

$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($userPath -notlike "*$binDir*") {
    [Environment]::SetEnvironmentVariable("Path", "$userPath;$binDir", "User")
    Write-Host "[install] Added to USER PATH: $binDir"
} else {
    Write-Host "[install] PATH already includes: $binDir"
}

Write-Host "[install] trinary installed to: $targetExe"
Write-Host "[install] Open a new terminal, then run: trinary --version"
