#!/usr/bin/env pwsh
# Download public Atari Lynx accuracy test ROMs
# Lynx has very limited public test ROM availability.
#
# Usage:
#   .\download-lynx-test-roms.ps1
#   .\download-lynx-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$lynxDir = Join-Path $RomRoot "lynx"

Write-Host "Downloading Atari Lynx test ROMs to: $lynxDir" -ForegroundColor Cyan

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

# --- cc65 Lynx samples (includes test programs) ---
Write-Host ""
Write-Host "=== cc65 Lynx Samples ===" -ForegroundColor White
Clone-Repo "https://github.com/cc65/cc65.git" (Join-Path $lynxDir "cc65") "cc65 (includes Lynx samples)"

# --- Lynx homebrew collection ---
Write-Host ""
Write-Host "=== Lynx Homebrew/Development Archive ===" -ForegroundColor White
Clone-Repo "https://github.com/42Bastian/new_bll.git" (Join-Path $lynxDir "new_bll") "new_bll (Bastian Lynx Library)"

Write-Host ""
Write-Host "Atari Lynx test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $lynxDir" -ForegroundColor Cyan
Write-Host ""
Write-Host "NOTE: Atari Lynx has very limited public test ROM coverage." -ForegroundColor DarkYellow
Write-Host "Consider creating custom test ROMs via the Flower Toolchain." -ForegroundColor DarkYellow
