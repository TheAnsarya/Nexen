#!/usr/bin/env pwsh
# Download public PCE (TurboGrafx-16/PC Engine) accuracy test ROMs
# PCE has limited public test ROMs compared to other systems.
#
# Usage:
#   .\download-pce-test-roms.ps1
#   .\download-pce-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$pceDir = Join-Path $RomRoot "pce"

Write-Host "Downloading PCE/TurboGrafx-16 test ROMs to: $pceDir" -ForegroundColor Cyan

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

# --- PeterLemon PCE tests ---
Write-Host ""
Write-Host "=== PeterLemon PCE Tests ===" -ForegroundColor White
Clone-Repo "https://github.com/PeterLemon/PCE.git" (Join-Path $pceDir "peterlemon-pce") "PeterLemon/PCE"

# --- PCEdev community resources ---
Write-Host ""
Write-Host "=== HuC Compiler/Tools (includes test ROMs) ===" -ForegroundColor White
Clone-Repo "https://github.com/pce-devel/huc.git" (Join-Path $pceDir "huc") "huc compiler/tools"

Write-Host ""
Write-Host "PCE/TurboGrafx-16 test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $pceDir" -ForegroundColor Cyan
Write-Host ""
Write-Host "NOTE: PCE has limited public test ROM coverage." -ForegroundColor DarkYellow
Write-Host "Consider creating custom test ROMs via the Flower Toolchain." -ForegroundColor DarkYellow
