#!/usr/bin/env pwsh
# Download public SMS (Sega Master System) accuracy test ROMs
# These are open-source/public-domain test ROMs used for emulator accuracy testing.
#
# Usage:
#   .\download-sms-test-roms.ps1
#   .\download-sms-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$smsDir = Join-Path $RomRoot "sms"

Write-Host "Downloading SMS test ROMs to: $smsDir" -ForegroundColor Cyan

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

# --- Flubba VDP tests ---
Write-Host ""
Write-Host "=== Flubba SMS VDP Tests ===" -ForegroundColor White
Clone-Repo "https://github.com/Kilian/Flubba-SMS-VDP-Test.git" (Join-Path $smsDir "flubba-vdp") "Flubba VDP"

# --- ZEXALL SMS (Z80 exerciser) ---
Write-Host ""
Write-Host "=== ZEXALL SMS Z80 Tests ===" -ForegroundColor White
Clone-Repo "https://github.com/superjamie/zexall-sms.git" (Join-Path $smsDir "zexall") "ZEXALL SMS"

# --- SMSPower community tests ---
Write-Host ""
Write-Host "=== PeterLemon SMS Tests ===" -ForegroundColor White
Clone-Repo "https://github.com/PeterLemon/SMS.git" (Join-Path $smsDir "peterlemon-sms") "PeterLemon/SMS"

Write-Host ""
Write-Host "SMS test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $smsDir" -ForegroundColor Cyan
