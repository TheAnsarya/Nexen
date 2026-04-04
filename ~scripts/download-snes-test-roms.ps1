#!/usr/bin/env pwsh
# Download public SNES accuracy test ROMs
#
# Usage:
#   .\download-snes-test-roms.ps1
#   .\download-snes-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$snesDir = Join-Path $RomRoot "snes"

Write-Host "Downloading SNES test ROMs to: $snesDir" -ForegroundColor Cyan

function Clone-Repo {
	param([string]$RepoUrl, [string]$TargetDir, [string]$Name)
	if (Test-Path $TargetDir) {
		Write-Host "  EXISTS: $TargetDir" -ForegroundColor DarkGray
		return
	}
	Write-Host "  CLONE: $Name" -ForegroundColor Yellow
	git clone --depth 1 $RepoUrl $TargetDir 2>&1 | Out-Null
	Write-Host "  OK: $TargetDir" -ForegroundColor Green
}

# --- PeterLemon SNES tests ---
Write-Host ""
Write-Host "=== PeterLemon/SNES ===" -ForegroundColor White
Clone-Repo "https://github.com/PeterLemon/SNES.git" (Join-Path $snesDir "peterlemon") "PeterLemon/SNES"

# --- undisbeliever snes-test-roms ---
Write-Host ""
Write-Host "=== undisbeliever/snes-test-roms ===" -ForegroundColor White
Clone-Repo "https://github.com/undisbeliever/snes-test-roms.git" (Join-Path $snesDir "undisbeliever") "undisbeliever/snes-test-roms"

# --- SNES-HiRomGsuTest ---
Write-Host ""
Write-Host "=== SNES-HiRomGsuTest ===" -ForegroundColor White
Clone-Repo "https://github.com/astrobleem/SNES-HiRomGsuTest.git" (Join-Path $snesDir "gsu-test") "SNES-HiRomGsuTest"

Write-Host ""
Write-Host "SNES test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $snesDir" -ForegroundColor Cyan
