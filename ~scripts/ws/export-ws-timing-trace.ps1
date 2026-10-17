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

$outputDir = Split-Path -Parent $OutputTracePath
if ($outputDir -and -not (Test-Path $outputDir)) {
	New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

$logDir = Split-Path -Parent $RawLogPath
if ($logDir -and -not (Test-Path $logDir)) {
	New-Item -ItemType Directory -Path $logDir -Force | Out-Null
}

# Use testRunner mode because it guarantees script execution lifecycle.
# Provide output path via environment variable for Lua file output fallback.
$oldEnv = $env:NEXEN_WS_TRACE_OUTPUT
$resolvedTracePath = (Resolve-Path $outputDir).Path
$resolvedTracePath = Join-Path $resolvedTracePath (Split-Path -Leaf $OutputTracePath)

if (Test-Path $resolvedTracePath) {
	Remove-Item -Path $resolvedTracePath -Force
}

$env:NEXEN_WS_TRACE_OUTPUT = $resolvedTracePath
try {
	$tmpStdout = Join-Path $env:TEMP ("nexen-ws-trace-stdout-" + [Guid]::NewGuid().ToString("N") + ".log")
	$tmpStderr = Join-Path $env:TEMP ("nexen-ws-trace-stderr-" + [Guid]::NewGuid().ToString("N") + ".log")

	$proc = Start-Process -FilePath $NexenPath -ArgumentList @(
		"--testRunner",
		"--enableStdout",
		"--timeout=60",
		$luaScript,
		$RomPath
	) -Wait -PassThru -NoNewWindow -RedirectStandardOutput $tmpStdout -RedirectStandardError $tmpStderr

	$runOutput = @()
	if (Test-Path $tmpStdout) {
		$runOutput += Get-Content -Path $tmpStdout
	}
	if (Test-Path $tmpStderr) {
		$stderrLines = Get-Content -Path $tmpStderr
		if ($stderrLines.Count -gt 0) {
			$runOutput += $stderrLines
		}
	}

	if (Test-Path $tmpStdout) { Remove-Item $tmpStdout -Force }
	if (Test-Path $tmpStderr) { Remove-Item $tmpStderr -Force }
} finally {
	$env:NEXEN_WS_TRACE_OUTPUT = $oldEnv
}
$runOutput | Set-Content -Path $RawLogPath -Encoding UTF8

if (Test-Path $resolvedTracePath) {
	$fileTraceLines = Get-Content -Path $resolvedTracePath | Where-Object { $_.Trim().Length -gt 0 }
	if ($fileTraceLines.Count -gt 0) {
		$fileTraceLines | Set-Content -Path $OutputTracePath -Encoding UTF8
		Write-Host "Exported $($fileTraceLines.Count) trace line(s)." -ForegroundColor Green
		exit 0
	}
}

$traceLines = @()
foreach ($line in $runOutput) {
	$lineText = [string]$line
	$idx = $lineText.IndexOf("ws-trace-json:")
	if ($idx -ge 0) {
		$traceLines += $lineText.Substring($idx + 14).Trim()
	}
}

if ($traceLines.Count -eq 0) {
	Write-Error "No ws-trace-json lines found in testRunner output."
	exit 1
}

$traceLines | Set-Content -Path $OutputTracePath -Encoding UTF8
Write-Host "Exported $($traceLines.Count) trace line(s)." -ForegroundColor Green
exit 0
