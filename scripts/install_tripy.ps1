<#
Copyright RomanAILabs - Daniel Harding (GitHub RomanAILabs-Auth)
Collaborators honored: Grok/xAI, Gemini-Flash/Google, ChatGPT-5.4/OpenAI, Cursor
Contact: daniel@romanailabs.com, romanailabs@gmail.com
Website: romanailabs.com
#>
[CmdletBinding()]
param(
    [switch]$UpgradePip
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$tripyRoot = Join-Path $root "tripy"

if ($UpgradePip) {
    python -m pip install --upgrade pip
    if ($LASTEXITCODE -ne 0) { throw "pip upgrade failed" }
}

python -m pip install -e $tripyRoot
if ($LASTEXITCODE -ne 0) { throw "tripy install failed" }

Write-Host "[install] tripy installed in editable mode from: $tripyRoot"
Write-Host "[install] Verify with: tripy --version"
