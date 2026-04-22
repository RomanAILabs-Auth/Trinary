# Activate the MSVC x64 developer environment for this PowerShell session.
# Usage:  . .\scripts\vcenv.ps1
# Idempotent — only runs vcvars64.bat once.

if ($env:TRINARY_VCVARS_LOADED -eq "1") { return }

$vsRoot = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$vcvars = Join-Path $vsRoot "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat not found at: $vcvars"
}

$envDump = cmd /c "`"$vcvars`" >NUL 2>&1 && set"
foreach ($line in $envDump) {
    if ($line -match '^([^=]+)=(.*)$') {
        [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2])
    }
}
$env:TRINARY_VCVARS_LOADED = "1"
Write-Host "[vcenv] MSVC x64 environment loaded." -ForegroundColor DarkGray
