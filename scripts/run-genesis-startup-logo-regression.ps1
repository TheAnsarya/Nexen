param(
	[string[]]$RomPaths = @(
		"C:\~reference-roms\genesis\md\Sonic The Hedgehog (W) (REV00) [!].md",
		"C:\~reference-roms\genesis\md\Sonic The Hedgehog (W) (REV01) [!].md",
		"C:\~reference-roms\genesis\md\Sonic The Hedgehog 2 (W) (REV01) [!].md"
	),
	[string]$CompareScriptPath = ".\scripts\run-genesis-cross-emu-wram-compare.ps1",
	[string]$OutputDirectory = ".\reference\startup-logo-regression",
	[string]$BaselinePath = ".\reference\startup-logo-baseline.json",
	[int]$FrameStart = 0,
	[int]$FrameEnd = 600,
	[int]$AutoStopTimeoutSeconds = 45,
	[int]$MaxLines = 1200000,
	[switch]$UpdateBaseline,
	[switch]$AllowMissingMesenFrontend,
	[switch]$DisableMesenFallbackRunModes
)

$ErrorActionPreference = "Stop"

function Resolve-PathRequired {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,
		[Parameter(Mandatory = $true)]
		[string]$Label
	)

	$resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
	if ($null -eq $resolved) {
		throw "Unable to resolve $Label path: $Path"
	}

	return $resolved.Path
}

function Resolve-PathOptional {
	param([string]$Path)

	if ([string]::IsNullOrWhiteSpace($Path)) {
		return $null
	}

	try {
		$resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
		if ($null -eq $resolved) {
			return $null
		}
		return $resolved.Path
	} catch {
		return $null
	}
}

function Convert-ReportToMap {
	param(
		[Parameter(Mandatory = $true)]
		[string]$ReportPath
	)

	$map = @{}
	foreach ($line in (Get-Content -LiteralPath $ReportPath -ErrorAction Stop)) {
		if ([string]::IsNullOrWhiteSpace($line)) {
			continue
		}

		$eq = $line.IndexOf('=')
		if ($eq -lt 1) {
			continue
		}

		$key = $line.Substring(0, $eq)
		$value = $line.Substring($eq + 1)
		$map[$key] = $value
	}

	return $map
}

function Get-BoolValue {
	param(
		$Value,
		[bool]$Default = $false
	)

	if ($null -eq $Value) {
		return $Default
	}

	$text = "$Value".Trim().ToLowerInvariant()
	if ($text -in @("1", "true", "yes", "on")) {
		return $true
	}
	if ($text -in @("0", "false", "no", "off")) {
		return $false
	}

	return $Default
}

function Get-IntValue {
	param(
		$Value,
		[int]$Default = 0
	)

	if ($null -eq $Value) {
		return $Default
	}

	$parsed = 0
	if ([int]::TryParse("$Value", [ref]$parsed)) {
		return $parsed
	}

	return $Default
}

function New-RomKey {
	param([string]$Path)

	if ([string]::IsNullOrWhiteSpace($Path)) {
		return "unknown"
	}

	$file = [System.IO.Path]::GetFileNameWithoutExtension($Path)
	if ([string]::IsNullOrWhiteSpace($file)) {
		$file = $Path
	}

	$normalized = [Regex]::Replace($file.ToLowerInvariant(), "[^a-z0-9]+", "-")
	return $normalized.Trim('-')
}

$resolvedCompareScriptPath = Resolve-PathRequired -Path $CompareScriptPath -Label "compare script"
$resolvedOutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
if (!(Test-Path -LiteralPath $resolvedOutputDirectory)) {
	New-Item -Path $resolvedOutputDirectory -ItemType Directory -Force | Out-Null
}

$resolvedBaselinePath = [System.IO.Path]::GetFullPath($BaselinePath)
$timestamp = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss")
$summaryJsonPath = Join-Path $resolvedOutputDirectory "startup-logo-regression-summary-$timestamp.json"
$summaryMarkdownPath = Join-Path $resolvedOutputDirectory "startup-logo-regression-summary-$timestamp.md"
$latestJsonPath = Join-Path $resolvedOutputDirectory "startup-logo-regression-summary.latest.json"
$latestMarkdownPath = Join-Path $resolvedOutputDirectory "startup-logo-regression-summary.latest.md"

$baseline = @{}
if ((Test-Path -LiteralPath $resolvedBaselinePath) -and -not $UpdateBaseline) {
	$baselineRaw = Get-Content -LiteralPath $resolvedBaselinePath -Raw -ErrorAction Stop
	if (-not [string]::IsNullOrWhiteSpace($baselineRaw)) {
		$baseline = $baselineRaw | ConvertFrom-Json -AsHashtable
	}
}

$results = New-Object System.Collections.Generic.List[object]
$regressionCount = 0
$missingMesenCount = 0
$anyErrors = $false

foreach ($rom in $RomPaths) {
	if ([string]::IsNullOrWhiteSpace($rom)) {
		continue
	}

	$resolvedRom = Resolve-PathOptional -Path $rom
	if ([string]::IsNullOrWhiteSpace($resolvedRom)) {
		Write-Warning "Skipping missing ROM path: $rom"
		continue
	}
	$romKey = New-RomKey -Path $resolvedRom
	$runReportPath = Join-Path $resolvedOutputDirectory "cross-emu-$romKey-$timestamp.txt"

	$runnerArgs = @(
		"-NoProfile",
		"-ExecutionPolicy", "Bypass",
		"-File", $resolvedCompareScriptPath,
		"-RomPath", $resolvedRom,
		"-FrameStart", "$FrameStart",
		"-FrameEnd", "$FrameEnd",
		"-AutoStopTimeoutSeconds", "$AutoStopTimeoutSeconds",
		"-MaxLines", "$MaxLines",
		"-ReportPath", $runReportPath
	)

	if ($AllowMissingMesenFrontend) {
		$runnerArgs += "-AllowMissingMesenFrontend"
	}
	if ($DisableMesenFallbackRunModes) {
		$runnerArgs += "-DisableMesenFallbackRunModes"
	}

	Write-Host "Running startup/logo compare for $resolvedRom" -ForegroundColor Cyan
	& pwsh @runnerArgs
	$compareExitCode = $LASTEXITCODE

	$report = Convert-ReportToMap -ReportPath $runReportPath
	$mesenMissing = Get-BoolValue -Value $report["mesenTraceMissing"]
	$mesenStartupMissing = Get-BoolValue -Value $report["mesenStartupTraceMissing"]
	$mesenSkipped = Get-BoolValue -Value $report["mesenRunSkipped"]

	if ($mesenMissing -or $mesenStartupMissing -or $mesenSkipped) {
		$missingMesenCount++
	}

	$current = [ordered]@{
		romKey = $romKey
		romPath = $resolvedRom
		reportPath = $runReportPath
		compareExitCode = $compareExitCode
		firstDiffIndex = Get-IntValue -Value $report["firstDiffIndex"] -Default -1
		startupFirstDiffIndex = Get-IntValue -Value $report["startupFirstDiffIndex"] -Default -1
		nexenHash = $report["nexenHash"]
		mesenHash = $report["mesenHash"]
		nexenStartupHash = $report["nexenStartupHash"]
		mesenStartupHash = $report["mesenStartupHash"]
		nexenLines = Get-IntValue -Value $report["nexenLines"]
		mesenLines = Get-IntValue -Value $report["mesenLines"]
		nexenStartupLines = Get-IntValue -Value $report["nexenStartupLines"]
		mesenStartupLines = Get-IntValue -Value $report["mesenStartupLines"]
		nexenStartupCheckpointCount = Get-IntValue -Value $report["nexenStartupCheckpointCount"]
		mesenStartupCheckpointCount = Get-IntValue -Value $report["mesenStartupCheckpointCount"]
		nexenStartupDisplayTransitionCount = Get-IntValue -Value $report["nexenStartupDisplayTransitionCount"]
		mesenStartupDisplayTransitionCount = Get-IntValue -Value $report["mesenStartupDisplayTransitionCount"]
		mesenTraceMissing = $mesenMissing
		mesenStartupTraceMissing = $mesenStartupMissing
		mesenRunSkipped = $mesenSkipped
	}

	$baselineEntry = $null
	if ($baseline.ContainsKey($romKey)) {
		$baselineEntry = $baseline[$romKey]
	}

	$regressions = New-Object System.Collections.Generic.List[string]
	if ($null -ne $baselineEntry) {
		if ($baselineEntry.nexenHash -and $baselineEntry.nexenHash -ne $current.nexenHash) {
			$regressions.Add("nexenHash")
		}
		if ($baselineEntry.nexenStartupHash -and $baselineEntry.nexenStartupHash -ne $current.nexenStartupHash) {
			$regressions.Add("nexenStartupHash")
		}
		if ($baselineEntry.nexenStartupCheckpointCount -ne $null -and [int]$baselineEntry.nexenStartupCheckpointCount -ne $current.nexenStartupCheckpointCount) {
			$regressions.Add("nexenStartupCheckpointCount")
		}
		if ($baselineEntry.nexenStartupDisplayTransitionCount -ne $null -and [int]$baselineEntry.nexenStartupDisplayTransitionCount -ne $current.nexenStartupDisplayTransitionCount) {
			$regressions.Add("nexenStartupDisplayTransitionCount")
		}
	}

	$current["regressions"] = @($regressions)
	$current["regressionCount"] = $regressions.Count
	if ($regressions.Count -gt 0) {
		$regressionCount++
	}

	if ($compareExitCode -ne 0 -and $compareExitCode -ne 3) {
		$anyErrors = $true
	}

	$results.Add([pscustomobject]$current)
}

$baselineOut = [ordered]@{}
foreach ($item in $results) {
	$baselineOut[$item.romKey] = [ordered]@{
		nexenHash = $item.nexenHash
		nexenStartupHash = $item.nexenStartupHash
		nexenStartupCheckpointCount = $item.nexenStartupCheckpointCount
		nexenStartupDisplayTransitionCount = $item.nexenStartupDisplayTransitionCount
	}
}

if ($UpdateBaseline) {
	$baselineJson = $baselineOut | ConvertTo-Json -Depth 6
	$baselineJson | Out-File -LiteralPath $resolvedBaselinePath -Encoding utf8
}

if ($results.Count -eq 0) {
	throw "No ROMs were resolved. Provide at least one valid -RomPaths entry."
}

$summary = New-Object PSObject
$resultArray = New-Object System.Collections.Generic.List[object]
foreach ($entry in $results) {
	$resultArray.Add($entry)
}
$summary | Add-Member -NotePropertyName "generatedUtc" -NotePropertyValue ([DateTime]::UtcNow.ToString("o"))
$summary | Add-Member -NotePropertyName "frameStart" -NotePropertyValue $FrameStart
$summary | Add-Member -NotePropertyName "frameEnd" -NotePropertyValue $FrameEnd
$summary | Add-Member -NotePropertyName "autoStopTimeoutSeconds" -NotePropertyValue $AutoStopTimeoutSeconds
$summary | Add-Member -NotePropertyName "maxLines" -NotePropertyValue $MaxLines
$summary | Add-Member -NotePropertyName "romCount" -NotePropertyValue $results.Count
$summary | Add-Member -NotePropertyName "regressionCount" -NotePropertyValue $regressionCount
$summary | Add-Member -NotePropertyName "missingMesenCount" -NotePropertyValue $missingMesenCount
$summary | Add-Member -NotePropertyName "anyErrors" -NotePropertyValue $anyErrors
$summary | Add-Member -NotePropertyName "baselinePath" -NotePropertyValue $resolvedBaselinePath
$summary | Add-Member -NotePropertyName "baselineUpdated" -NotePropertyValue ([bool]$UpdateBaseline)
$summary | Add-Member -NotePropertyName "results" -NotePropertyValue $resultArray.ToArray()

$summaryJson = $summary | ConvertTo-Json -Depth 8
$summaryJson | Out-File -LiteralPath $summaryJsonPath -Encoding utf8
$summaryJson | Out-File -LiteralPath $latestJsonPath -Encoding utf8

$md = New-Object System.Collections.Generic.List[string]
$md.Add("# Genesis Startup Logo Regression Summary")
$md.Add("")
$md.Add("- generatedUtc: $($summary.generatedUtc)")
$md.Add("- frameStart: $FrameStart")
$md.Add("- frameEnd: $FrameEnd")
$md.Add("- autoStopTimeoutSeconds: $AutoStopTimeoutSeconds")
$md.Add("- maxLines: $MaxLines")
$md.Add("- romCount: $($summary.romCount)")
$md.Add("- regressionCount: $regressionCount")
$md.Add("- missingMesenCount: $missingMesenCount")
$md.Add("- anyErrors: $anyErrors")
$md.Add("")
$md.Add("| ROM | Exit | Diff | Startup Diff | Nexen Startup Hash | Mesen Startup Hash | Nexen Chkpt | Nexen Disp Tgl | Regressions |")
$md.Add("|---|---:|---:|---:|---|---|---:|---:|---|")
foreach ($item in $results) {
	$regressionText = ""
	if ($item.regressionCount -gt 0) {
		$regressionText = ($item.regressions -join ",")
	}

	$md.Add("| $($item.romKey) | $($item.compareExitCode) | $($item.firstDiffIndex) | $($item.startupFirstDiffIndex) | $($item.nexenStartupHash) | $($item.mesenStartupHash) | $($item.nexenStartupCheckpointCount) | $($item.nexenStartupDisplayTransitionCount) | $regressionText |")
}

$md.Add("")
$md.Add("## Notes")
$md.Add("")
$md.Add("- compareExitCode=3 indicates first-difference detection from the compare script.")
$md.Add("- missingMesenCount includes missing trace/startup-trace or skipped Mesen run.")
$md.Add("- Use -UpdateBaseline to refresh baseline hashes and startup counters.")

$md | Out-File -LiteralPath $summaryMarkdownPath -Encoding utf8
$md | Out-File -LiteralPath $latestMarkdownPath -Encoding utf8

Write-Host "Startup/logo summary JSON: $summaryJsonPath" -ForegroundColor Green
Write-Host "Startup/logo summary Markdown: $summaryMarkdownPath" -ForegroundColor Green
Write-Host "Latest JSON: $latestJsonPath" -ForegroundColor Green
Write-Host "Latest Markdown: $latestMarkdownPath" -ForegroundColor Green

if ($anyErrors) {
	exit 4
}

if ($regressionCount -gt 0) {
	exit 5
}

exit 0
