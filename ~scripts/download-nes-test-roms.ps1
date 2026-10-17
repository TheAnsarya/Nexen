#!/usr/bin/env pwsh
# Download public NES accuracy test ROMs
# These are open-source/public-domain test ROMs used for emulator accuracy testing.
#
# Usage:
#   .\download-nes-test-roms.ps1
#   .\download-nes-test-roms.ps1 -RomRoot "C:\~reference-roms"

[CmdletBinding()]
param(
	[string]$RomRoot = "C:\~reference-roms"
)

$ErrorActionPreference = "Stop"
$nesDir = Join-Path $RomRoot "nes"

Write-Host "Downloading NES test ROMs to: $nesDir" -ForegroundColor Cyan

# Helper to download a file
function Download-File {
	param([string]$Url, [string]$OutPath)
	$dir = Split-Path $OutPath -Parent
	if (-not (Test-Path $dir)) {
		New-Item -ItemType Directory -Path $dir -Force | Out-Null
	}
	if (Test-Path $OutPath) {
		Write-Host "  EXISTS: $OutPath" -ForegroundColor DarkGray
		return
	}
	Write-Host "  GET: $Url" -ForegroundColor Yellow
	Invoke-WebRequest -Uri $Url -OutFile $OutPath -UseBasicParsing
	Write-Host "  OK: $OutPath" -ForegroundColor Green
}

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

# --- blargg test ROMs ---
Write-Host ""
Write-Host "=== blargg NES test ROMs ===" -ForegroundColor White

$blarggBase = "https://raw.githubusercontent.com/christopherpow/nes-test-roms/master"

# cpu_instrs
$cpuTests = @(
	"01-basics", "02-implied", "03-immediate", "04-zero_page",
	"05-zp_xy", "06-absolute", "07-abs_xy", "08-ind_x",
	"09-ind_y", "10-branches", "11-stack"
)
foreach ($t in $cpuTests) {
	Download-File "$blarggBase/blargg_cpu_instrs/individual/$t.nes" (Join-Path $nesDir "blargg\cpu_instrs\individual\$t.nes")
}
Download-File "$blarggBase/blargg_cpu_instrs/cpu_instrs.nes" (Join-Path $nesDir "blargg\cpu_instrs\cpu_instrs.nes")

# ppu_vbl_nmi
$ppuTests = @(
	"01-vbl_basics", "02-vbl_set_time", "03-vbl_clear_time",
	"04-nmi_control", "05-nmi_timing", "06-suppression",
	"07-nmi_on_timing", "08-nmi_off_timing", "09-even_odd_frames", "10-even_odd_timing"
)
foreach ($t in $ppuTests) {
	Download-File "$blarggBase/blargg_ppu_tests_2005.09.15b/individual/$t.nes" (Join-Path $nesDir "blargg\ppu_vbl_nmi\individual\$t.nes")
}

# --- AccuracyCoin ---
Write-Host ""
Write-Host "=== AccuracyCoin ===" -ForegroundColor White
Clone-Repo "https://github.com/100thCoin/AccuracyCoin.git" (Join-Path $nesDir "accuracycoin") "AccuracyCoin"

Write-Host ""
Write-Host "NES test ROM download complete." -ForegroundColor Green
Write-Host "ROM root: $nesDir" -ForegroundColor Cyan
