<#
Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
Contact: daniel@romanailabs.com, romanailabs@gmail.com
Website: romanailabs.com
#>
[CmdletBinding()]
param(
    [switch]$RunTests
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent

function Assert-File {
    param([string]$Path)
    if (-not (Test-Path $Path)) {
        throw "missing required file: $Path"
    }
}

function Assert-Contains {
    param([string]$Path, [string]$Needle)
    $content = Get-Content -LiteralPath $Path -Raw
    if ($content -notmatch [Regex]::Escape($Needle)) {
        throw "required text not found in $Path : $Needle"
    }
}

Write-Host "[release-check] verifying required files..." -ForegroundColor Cyan
Assert-File (Join-Path $root "LICENSE")
Assert-File (Join-Path $root "NOTICE")
Assert-File (Join-Path $root "README.md")
Assert-File (Join-Path $root "engine\README.md")
Assert-File (Join-Path $root "tripy\README.md")
Assert-File (Join-Path $root "scripts\install_trinary.ps1")
Assert-File (Join-Path $root "scripts\install_tripy.ps1")
Assert-File (Join-Path $root "scripts\install_trinary.sh")
Assert-File (Join-Path $root "scripts\install_tripy.sh")
Assert-File (Join-Path $root "trinary-tripy-architecture.md")

Write-Host "[release-check] verifying licensing/attribution markers..." -ForegroundColor Cyan
Assert-Contains (Join-Path $root "LICENSE") "RomanAILabs Business Source License 1.1"
Assert-Contains (Join-Path $root "NOTICE") "ChatGPT-5.4 / OpenAI"
Assert-Contains (Join-Path $root "README.md") "ChatGPT-5.4 / OpenAI"
Assert-Contains (Join-Path $root "tripy\README.md") "ChatGPT-5.4 / OpenAI"
Assert-Contains (Join-Path $root "engine\README.md") "ChatGPT-5.4 / OpenAI"

Write-Host "[release-check] verifying architecture/license consistency..." -ForegroundColor Cyan
Assert-Contains (Join-Path $root "trinary-tripy-architecture.md") "RomanAILabs Business Source License 1.1"

if ($RunTests) {
    Write-Host "[release-check] running full build + tests..." -ForegroundColor Cyan
    & (Join-Path $root "build.ps1") -Config Release -Tests
    if ($LASTEXITCODE -ne 0) {
        throw "build/tests failed"
    }
}

Write-Host "[release-check] OK" -ForegroundColor Green
