#!/usr/bin/env pwsh
# Download public Game Boy accuracy test ROMs
#
# Usage:
#   .\download-gb-test-roms.ps1
#   .\download-gb-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$gbDir = Join-Path $RomRoot "gb"

Write-Host "Downloading GB test ROMs to: $gbDir" -ForegroundColor Cyan

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

# --- Mooneye Test Suite ---
Write-Host ""
Write-Host "=== Mooneye Test Suite ===" -ForegroundColor White
Clone-Repo "https://github.com/Gekkio/mooneye-test-suite.git" (Join-Path $gbDir "mooneye") "Mooneye"

# --- dmg-acid2 ---
Write-Host ""
Write-Host "=== dmg-acid2 / cgb-acid2 ===" -ForegroundColor White
Clone-Repo "https://github.com/mattcurrie/dmg-acid2.git" (Join-Path $gbDir "dmg-acid2") "dmg-acid2"
Clone-Repo "https://github.com/mattcurrie/cgb-acid2.git" (Join-Path $gbDir "cgb-acid2") "cgb-acid2"

# --- SameSuite ---
Write-Host ""
Write-Host "=== SameSuite ===" -ForegroundColor White
Clone-Repo "https://github.com/LIJI32/SameSuite.git" (Join-Path $gbDir "samesuite") "SameSuite"

Write-Host ""
Write-Host "GB test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $gbDir" -ForegroundColor Cyan
