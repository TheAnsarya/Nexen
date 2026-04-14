#!/usr/bin/env pwsh
# Run WS timing trace Lua script and export ws-trace-json lines to a JSONL file.
#
# Usage:
#   .\export-ws-timing-trace.ps1 -RomPath "C:\~reference-roms\custom\ws\ws-timing-order-scaffold.ws"
#   .\export-ws-timing-trace.ps1 -RomPath "..." -OutputTracePath ".\ws-nexen-trace.jsonl"

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$RomPath,
	[string]$NexenPath,
	[string]$OutputTracePath = "$PSScriptRoot\ws-timing-trace.nexen.jsonl",
	[string]$RawLogPath = "$PSScriptRoot\ws-timing-trace.nexen.log"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $RomPath)) {
	Write-Error "ROM not found: $RomPath"
	exit 1
}

if (-not $NexenPath) {
	$candidates = @(
		(Join-Path $PSScriptRoot "..\..\bin\win-x64\Release\Nexen.exe"),
		(Join-Path $PSScriptRoot "..\..\bin\win-x64\Debug\Nexen.exe")
	)
	foreach ($candidate in $candidates) {
		if (Test-Path $candidate) {
			$NexenPath = (Resolve-Path $candidate).Path
			break
		}
	}
}

if (-not $NexenPath -or -not (Test-Path $NexenPath)) {
	Write-Error "Nexen executable not found. Specify -NexenPath."
	exit 1
}

$luaScript = Join-Path $PSScriptRoot "ws-timing-trace.lua"
if (-not (Test-Path $luaScript)) {
	Write-Error "Lua script not found: $luaScript"
	exit 1
}

Write-Host "Nexen: $NexenPath" -ForegroundColor Cyan
Write-Host "ROM: $RomPath" -ForegroundColor Cyan
Write-Host "Raw log: $RawLogPath" -ForegroundColor Cyan
Write-Host "Trace output: $OutputTracePath" -ForegroundColor Cyan

$runOutput = & $NexenPath --novideo --noaudio --noinput --lua $luaScript $RomPath 2>&1
$runOutput | Set-Content -Path $RawLogPath -Encoding UTF8

$traceLines = @()
foreach ($line in $runOutput) {
	$lineText = [string]$line
	$idx = $lineText.IndexOf("ws-trace-json:")
	if ($idx -ge 0) {
		$traceLines += $lineText.Substring($idx + 14).Trim()
	}
}

if ($traceLines.Count -eq 0) {
	Write-Error "No ws-trace-json lines found in log output."
	exit 1
}

$traceLines | Set-Content -Path $OutputTracePath -Encoding UTF8
Write-Host "Exported $($traceLines.Count) trace line(s)." -ForegroundColor Green
exit 0
