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

function Assert-Ordering {
	param(
		[object[]]$Trace,
		[string]$Label
	)

	if ($Trace.Count -lt 2) {
		throw "$Label trace has fewer than 2 entries; multi-event timeline required."
	}

	$wrapCount = 0
	for ($i = 0; $i -lt $Trace.Count; $i++) {
		$entry = $Trace[$i]
		if ($entry.index -ne $i) {
			throw "$Label trace index ordering invalid at position $i (entry.index=$($entry.index))."
		}

		if ($entry.eventId -eq 0xff) {
			throw "$Label trace uses fallback eventId 0xff; expected deterministic multi-event timeline."
		}

		if ($i -gt 0) {
			$prev = $Trace[$i - 1]
			if ($entry.scanline -lt $prev.scanline) {
				$wrapCount++
			}
		}
	}

	if ($wrapCount -gt 1) {
		throw "$Label trace scanline ordering invalid (more than one wrap detected)."
	}
}

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

Assert-Ordering -Trace $nexen -Label "Nexen"
Assert-Ordering -Trace $reference -Label "Reference"

Write-Host "Nexen trace entries: $($nexen.Count)" -ForegroundColor Cyan
Write-Host "Reference entries: $($reference.Count)" -ForegroundColor Cyan

$mismatchCount = 0
$count = [Math]::Min($nexen.Count, $reference.Count)

for ($i = 0; $i -lt $count; $i++) {
	$n = $nexen[$i]
	$r = $reference[$i]

	$nIrqActive = if ($null -ne $n.PSObject.Properties['irqActive']) { $n.irqActive } else { $n.data0 }
	$rIrqActive = if ($null -ne $r.PSObject.Properties['irqActive']) { $r.irqActive } else { $r.data0 }
	$nIrqEnabled = if ($null -ne $n.PSObject.Properties['irqEnabled']) { $n.irqEnabled } else { 0 }
	$rIrqEnabled = if ($null -ne $r.PSObject.Properties['irqEnabled']) { $r.irqEnabled } else { 0 }
	$nHtimer = if ($null -ne $n.PSObject.Properties['hTimer']) { $n.hTimer } else { $n.data1 }
	$rHtimer = if ($null -ne $r.PSObject.Properties['hTimer']) { $r.hTimer } else { $r.data1 }
	$nVtimer = if ($null -ne $n.PSObject.Properties['vTimer']) { $n.vTimer } else { 0 }
	$rVtimer = if ($null -ne $r.PSObject.Properties['vTimer']) { $r.vTimer } else { 0 }

	$matched = ($n.eventId -eq $r.eventId) -and
		($n.scanline -eq $r.scanline) -and
		($nIrqActive -eq $rIrqActive) -and
		($nIrqEnabled -eq $rIrqEnabled) -and
		($nHtimer -eq $rHtimer) -and
		($nVtimer -eq $rVtimer)

	if (-not $matched) {
		$mismatchCount++
		if ($mismatchCount -le $MaxMismatchReports) {
			Write-Host ("Mismatch[{0}] idx={1}: nexen(e={2}, l={3}, ia={4}, ie={5}, ht={6}, vt={7}) ref(e={8}, l={9}, ia={10}, ie={11}, ht={12}, vt={13})" -f `
				$mismatchCount, $i, $n.eventId, $n.scanline, $nIrqActive, $nIrqEnabled, $nHtimer, $nVtimer, $r.eventId, $r.scanline, $rIrqActive, $rIrqEnabled, $rHtimer, $rVtimer) -ForegroundColor Red
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
