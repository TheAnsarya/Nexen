#!/usr/bin/env pwsh
# One-command WonderSwan timing regression gate for #1076/#1285.
#
# Workflow:
#   1) Build custom ROMs (includes ws-timing-order-scaffold)
#   2) Export Nexen timing trace from scaffold ROM
#   3) Compare trace against checked-in reference JSONL
#
# CI-friendly behavior:
#   - Exit 0 on pass
#   - Exit 1 on mismatch/error
#
# Usage:
#   .\run-ws-timing-gate.ps1
#   .\run-ws-timing-gate.ps1 -NexenPath "C:\path\Nexen.exe"
#   .\run-ws-timing-gate.ps1 -UpdateReference

[CmdletBinding()]
param(
	[string]$NexenPath,
	[string]$PoppyPath = "",
	[string]$ReferenceTracePath = "$PSScriptRoot\reference\ws-timing-order-scaffold.reference.jsonl",
	[switch]$UpdateReference
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$buildScript = Join-Path $root "~scripts\build-custom-roms.ps1"
$exportScript = Join-Path $root "~scripts\ws\export-ws-timing-trace.ps1"
$compareScript = Join-Path $root "~scripts\ws\compare-ws-timing-traces.ps1"
$romPath = Join-Path $root "~scripts\custom-roms\bin\ws\ws-timing-order-scaffold.ws"

if (-not (Test-Path $buildScript)) {
	Write-Error "Build script not found: $buildScript"
	exit 1
}

if (-not (Test-Path $exportScript)) {
	Write-Error "Trace export script not found: $exportScript"
	exit 1
}

if (-not (Test-Path $compareScript)) {
	Write-Error "Trace compare script not found: $compareScript"
	exit 1
}

Write-Host "[ws-gate] Building custom ROMs..." -ForegroundColor Cyan
if ($PoppyPath) {
	& $buildScript -PoppyPath $PoppyPath
} else {
	& $buildScript
}
if ($LASTEXITCODE -ne 0) {
	Write-Error "Custom ROM build failed."
	exit 1
}

if (-not (Test-Path $romPath)) {
	Write-Error "Scaffold ROM not found after build: $romPath"
	exit 1
}

$tempDir = Join-Path $env:TEMP "nexen-ws-timing-gate"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

$traceOut = Join-Path $tempDir "ws-timing-order-scaffold.nexen.jsonl"
$rawLogOut = Join-Path $tempDir "ws-timing-order-scaffold.nexen.log"

Write-Host "[ws-gate] Exporting Nexen trace..." -ForegroundColor Cyan
if ($NexenPath) {
	& $exportScript -RomPath $romPath -NexenPath $NexenPath -OutputTracePath $traceOut -RawLogPath $rawLogOut
} else {
	& $exportScript -RomPath $romPath -OutputTracePath $traceOut -RawLogPath $rawLogOut
}
if ($LASTEXITCODE -ne 0) {
	Write-Error "Trace export failed."
	exit 1
}

$traceLineCount = @(Get-Content -Path $traceOut | Where-Object { $_.Trim().Length -gt 0 }).Count
if ($traceLineCount -lt 2) {
	Write-Error "Exported trace has fewer than 2 entries ($traceLineCount). Multi-event timeline required."
	exit 1
}

if ($UpdateReference) {
	$refDir = Split-Path -Parent $ReferenceTracePath
	New-Item -ItemType Directory -Path $refDir -Force | Out-Null
	Copy-Item -Path $traceOut -Destination $ReferenceTracePath -Force
	Write-Host "[ws-gate] Reference trace updated: $ReferenceTracePath" -ForegroundColor Yellow
	Write-Host "[ws-gate] PASS (reference updated)." -ForegroundColor Green
	exit 0
}

if (-not (Test-Path $ReferenceTracePath)) {
	Write-Error "Reference trace not found: $ReferenceTracePath"
	Write-Host "[ws-gate] Generate it once with -UpdateReference." -ForegroundColor Yellow
	exit 1
}

Write-Host "[ws-gate] Comparing against reference..." -ForegroundColor Cyan
& $compareScript -NexenTracePath $traceOut -ReferenceTracePath $ReferenceTracePath
$compareExit = $LASTEXITCODE

if ($compareExit -eq 0) {
	Write-Host "[ws-gate] PASS" -ForegroundColor Green
	exit 0
}

Write-Host "[ws-gate] FAIL" -ForegroundColor Red
exit 1
