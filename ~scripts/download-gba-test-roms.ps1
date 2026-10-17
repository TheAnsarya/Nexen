#!/usr/bin/env pwsh
# Download public GBA accuracy test ROMs
#
# Usage:
#   .\download-gba-test-roms.ps1
#   .\download-gba-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$gbaDir = Join-Path $RomRoot "gba"

Write-Host "Downloading GBA test ROMs to: $gbaDir" -ForegroundColor Cyan

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

# --- jsmolka/gba-tests ---
Write-Host ""
Write-Host "=== jsmolka/gba-tests ===" -ForegroundColor White
Clone-Repo "https://github.com/jsmolka/gba-tests.git" (Join-Path $gbaDir "jsmolka") "jsmolka/gba-tests"

# --- mGBA test suite ---
Write-Host ""
Write-Host "=== mgba-emu/suite ===" -ForegroundColor White
Clone-Repo "https://github.com/mgba-emu/suite.git" (Join-Path $gbaDir "mgba-suite") "mgba-emu/suite"

Write-Host ""
Write-Host "GBA test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $gbaDir" -ForegroundColor Cyan
