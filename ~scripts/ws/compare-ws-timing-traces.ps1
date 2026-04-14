#!/usr/bin/env pwsh
# Compare two WonderSwan timing trace JSONL files.
#
# Usage:
#   .\compare-ws-timing-traces.ps1 -NexenTracePath .\ws-nexen.jsonl -ReferenceTracePath .\ws-ares.jsonl

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$NexenTracePath,
	[Parameter(Mandatory = $true)]
	[string]$ReferenceTracePath,
	[int]$MaxMismatchReports = 20
)

$ErrorActionPreference = "Stop"

function Read-TraceJsonl {
	param([string]$Path)
	if (-not (Test-Path $Path)) {
		throw "Trace file not found: $Path"
	}

	$items = @()
	$lineNo = 0
	foreach ($line in (Get-Content -Path $Path)) {
		$lineNo++
		$trim = $line.Trim()
		if ($trim.Length -eq 0) {
			continue
		}
		try {
			$obj = $trim | ConvertFrom-Json
			$items += $obj
		} catch {
			throw "Invalid JSON at $Path line ${lineNo}: $trim"
		}
	}
	return $items
}

$nexen = Read-TraceJsonl -Path $NexenTracePath
$reference = Read-TraceJsonl -Path $ReferenceTracePath

Write-Host "Nexen trace entries: $($nexen.Count)" -ForegroundColor Cyan
Write-Host "Reference entries: $($reference.Count)" -ForegroundColor Cyan

$mismatchCount = 0
$count = [Math]::Min($nexen.Count, $reference.Count)

for ($i = 0; $i -lt $count; $i++) {
	$n = $nexen[$i]
	$r = $reference[$i]

	$matched = ($n.eventId -eq $r.eventId) -and
		($n.scanline -eq $r.scanline) -and
		($n.data0 -eq $r.data0) -and
		($n.data1 -eq $r.data1)

	if (-not $matched) {
		$mismatchCount++
		if ($mismatchCount -le $MaxMismatchReports) {
			Write-Host ("Mismatch[{0}] idx={1}: nexen(e={2}, l={3}, d0={4}, d1={5}) ref(e={6}, l={7}, d0={8}, d1={9})" -f `
				$mismatchCount, $i, $n.eventId, $n.scanline, $n.data0, $n.data1, $r.eventId, $r.scanline, $r.data0, $r.data1) -ForegroundColor Red
		}
	}
}

if ($nexen.Count -ne $reference.Count) {
	$mismatchCount += [Math]::Abs($nexen.Count - $reference.Count)
	Write-Host "Entry count differs: nexen=$($nexen.Count), reference=$($reference.Count)" -ForegroundColor Red
}

if ($mismatchCount -eq 0) {
	Write-Host "Trace compare PASS: traces match exactly." -ForegroundColor Green
	exit 0
}

Write-Host "Trace compare FAIL: mismatches=$mismatchCount" -ForegroundColor Red
exit 1
