#!/usr/bin/env pwsh
# Download public WonderSwan accuracy test ROMs
# WonderSwan has very limited public test ROM availability.
#
# Usage:
#   .\download-ws-test-roms.ps1
#   .\download-ws-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$wsDir = Join-Path $RomRoot "ws"

Write-Host "Downloading WonderSwan test ROMs to: $wsDir" -ForegroundColor Cyan

# Helper to clone a repo
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

# --- WSdev community tests ---
Write-Host ""
Write-Host "=== WSdev WonderSwan Tools/Tests ===" -ForegroundColor White
Clone-Repo "https://github.com/asiekierka/ws-test-suite.git" (Join-Path $wsDir "ws-test-suite") "ws-test-suite"

# --- WonderSwan homebrew/test collection ---
Write-Host ""
Write-Host "=== Wonderful Toolchain (includes test programs) ===" -ForegroundColor White
Clone-Repo "https://github.com/WonderfulToolchain/wonderful-i-hate-it.git" (Join-Path $wsDir "wonderful") "Wonderful Toolchain"

Write-Host ""
Write-Host "WonderSwan test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $wsDir" -ForegroundColor Cyan
Write-Host ""
Write-Host "NOTE: WonderSwan has very limited public test ROM coverage." -ForegroundColor DarkYellow
Write-Host "Consider creating custom test ROMs via the Flower Toolchain." -ForegroundColor DarkYellow
