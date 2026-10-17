#!/usr/bin/env pwsh
# Download public Genesis/Mega Drive accuracy test ROMs
# These are open-source/public-domain test ROMs used for emulator accuracy testing.
#
# Usage:
#   .\download-genesis-test-roms.ps1
#   .\download-genesis-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$genesisDir = Join-Path $RomRoot "genesis"

Write-Host "Downloading Genesis/Mega Drive test ROMs to: $genesisDir" -ForegroundColor Cyan

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

# --- PeterLemon/MD ---
Write-Host ""
Write-Host "=== PeterLemon MD Test ROMs ===" -ForegroundColor White
Clone-Repo "https://github.com/PeterLemon/MD.git" (Join-Path $genesisDir "peterlemon-md") "PeterLemon/MD"

# --- BlastEm test suite ---
Write-Host ""
Write-Host "=== BlastEm Test ROMs ===" -ForegroundColor White
Clone-Repo "https://github.com/micky418/blastem-test-roms.git" (Join-Path $genesisDir "blastem-tests") "BlastEm tests"

# --- ZEXALL (Z80 exerciser for Genesis) ---
Write-Host ""
Write-Host "=== ZEXALL Z80 Tests ===" -ForegroundColor White
Clone-Repo "https://github.com/superjamie/zexall-sms.git" (Join-Path $genesisDir "zexall") "ZEXALL"

# --- Genesis-test-runner (community tests) ---
Write-Host ""
Write-Host "=== Genesis Plus GX Test ROMs ===" -ForegroundColor White
Clone-Repo "https://github.com/flamewing/mdcomp-test-roms.git" (Join-Path $genesisDir "mdcomp-tests") "mdcomp test ROMs"

Write-Host ""
Write-Host "Genesis/Mega Drive test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $genesisDir" -ForegroundColor Cyan
