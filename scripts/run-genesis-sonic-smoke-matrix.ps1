param(
	[Parameter(Mandatory = $true)]
	[string[]]$RomPaths,
	[string]$ExePath = ".\bin\win-x64\Release\Nexen.exe",
	[int]$AutoStopTimeoutSeconds = 8,
	[string]$NonSonicRomPath,
	[string]$SummaryJsonPath,
	[string]$SummaryMarkdownPath,
	[string]$SummaryCsvPath,
	[string]$CompactSummaryArtifactPath,
	[int]$RetryCount = 0,
	[int]$ForceProbeFailureCountPerRom = 0,
	[int]$ForceProbeFailureEveryN = 0,
	[switch]$StopOnFirstFailure,
	[switch]$VerboseLaunch,
	[switch]$TimestampedArtifacts,
	[switch]$CreateLatestAliases,
	[switch]$CompactCiSummary,
	[switch]$ForceFirstAttemptFailureProbe
)

function Get-ArtifactPath {
	param(
		[string]$BasePath,
		[string]$RunTimestamp,
		[bool]$UseTimestampedSuffix
	)

	if ([string]::IsNullOrWhiteSpace($BasePath)) {
		return $null
	}

	if (-not $UseTimestampedSuffix) {
		return $BasePath
	}

	$directory = Split-Path -Path $BasePath -Parent
	$fileName = [System.IO.Path]::GetFileNameWithoutExtension($BasePath)
	$extension = [System.IO.Path]::GetExtension($BasePath)
	$timestampedName = "$fileName-$RunTimestamp$extension"

	if ([string]::IsNullOrWhiteSpace($directory)) {
		return $timestampedName
	}

	return Join-Path $directory $timestampedName
}

function Get-LatestAliasPath {
	param([string]$BasePath)

	if ([string]::IsNullOrWhiteSpace($BasePath)) {
		return $null
	}

	$directory = Split-Path -Path $BasePath -Parent
	$fileName = [System.IO.Path]::GetFileNameWithoutExtension($BasePath)
	$extension = [System.IO.Path]::GetExtension($BasePath)
	$latestName = "$fileName-latest$extension"

	if ([string]::IsNullOrWhiteSpace($directory)) {
		return $latestName
	}

	return Join-Path $directory $latestName
}

function Ensure-ParentDirectory {
	param([string]$TargetPath)

	if ([string]::IsNullOrWhiteSpace($TargetPath)) {
		return
	}

	$targetDir = Split-Path -Path $TargetPath -Parent
	if (-not [string]::IsNullOrWhiteSpace($targetDir)) {
		New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
	}
}

function Write-LatestAlias {
	param(
		[string]$ActualPath,
		[string]$BasePath,
		[string]$Label,
		[bool]$Enabled
	)

	if (-not $Enabled -or [string]::IsNullOrWhiteSpace($ActualPath) -or [string]::IsNullOrWhiteSpace($BasePath)) {
		return
	}

	if (-not (Test-Path -LiteralPath $ActualPath)) {
		return
	}

	$latestAlias = Get-LatestAliasPath -BasePath $BasePath
	Ensure-ParentDirectory -TargetPath $latestAlias
	Copy-Item -LiteralPath $ActualPath -Destination $latestAlias -Force
	Write-Host "$Label latest alias: $latestAlias" -ForegroundColor Cyan
}

$runner = Join-Path $PSScriptRoot "run-genesis-sonic-smoke.ps1"
if (-not (Test-Path -LiteralPath $runner)) {
	Write-Error "Missing smoke runner script: $runner"
	exit 10
}

if (-not (Test-Path -LiteralPath $ExePath)) {
	Write-Error "Missing Nexen executable path: $ExePath"
	exit 11
}

$allRomPaths = New-Object System.Collections.Generic.List[string]
foreach ($rom in $RomPaths) {
	$allRomPaths.Add($rom)
}

if (-not [string]::IsNullOrWhiteSpace($NonSonicRomPath)) {
	$allRomPaths.Add($NonSonicRomPath)
}

foreach ($rom in $allRomPaths) {
	if (-not (Test-Path -LiteralPath $rom)) {
		Write-Error "Missing ROM path: $rom"
		exit 12
	}
}

$failed = New-Object System.Collections.Generic.List[string]
$results = New-Object System.Collections.Generic.List[object]
$runTimestamp = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss")
$syntheticFailureCount = 0

$jsonOutputPath = Get-ArtifactPath -BasePath $SummaryJsonPath -RunTimestamp $runTimestamp -UseTimestampedSuffix:$TimestampedArtifacts
$markdownOutputPath = Get-ArtifactPath -BasePath $SummaryMarkdownPath -RunTimestamp $runTimestamp -UseTimestampedSuffix:$TimestampedArtifacts
$csvOutputPath = Get-ArtifactPath -BasePath $SummaryCsvPath -RunTimestamp $runTimestamp -UseTimestampedSuffix:$TimestampedArtifacts
$compactSummaryOutputPath = Get-ArtifactPath -BasePath $CompactSummaryArtifactPath -RunTimestamp $runTimestamp -UseTimestampedSuffix:$TimestampedArtifacts
$compactSummaryLatestAliasPath = $null
if ($CreateLatestAliases -and -not [string]::IsNullOrWhiteSpace($CompactSummaryArtifactPath)) {
	$compactSummaryLatestAliasPath = Get-LatestAliasPath -BasePath $CompactSummaryArtifactPath
}

Write-Host "Running Genesis Sonic smoke matrix for $($allRomPaths.Count) ROM(s)..." -ForegroundColor Cyan

foreach ($rom in $allRomPaths) {
	Write-Host "---" -ForegroundColor DarkGray
	Write-Host "ROM: $rom" -ForegroundColor Cyan
	$romStopwatch = [System.Diagnostics.Stopwatch]::StartNew()

	$attempt = 0
	$passed = $false
	$exitCode = 0
	$maxAttempts = 1 + [Math]::Max(0, $RetryCount)
	$lastAttemptElapsedMs = [int64]0
	$attemptElapsedMsValues = New-Object System.Collections.Generic.List[int64]
	$syntheticFailuresInjectedForRom = 0
	while ($attempt -lt $maxAttempts -and -not $passed) {
		$attempt++
		if ($attempt -gt 1) {
			Write-Host "Retry $attempt/$maxAttempts for $rom" -ForegroundColor Yellow
		}

		$shouldInjectSyntheticFailure = $false
		if ($ForceProbeFailureCountPerRom -gt 0 -and $syntheticFailuresInjectedForRom -lt $ForceProbeFailureCountPerRom) {
			$shouldInjectSyntheticFailure = $true
		} elseif ($ForceFirstAttemptFailureProbe -and $attempt -eq 1) {
			$shouldInjectSyntheticFailure = $true
		} elseif ($ForceProbeFailureEveryN -gt 0 -and ($attempt % $ForceProbeFailureEveryN) -eq 0) {
			$shouldInjectSyntheticFailure = $true
		}

		if ($shouldInjectSyntheticFailure) {
			$exitCode = 99
			$lastAttemptElapsedMs = [int64]0
			$attemptElapsedMsValues.Add($lastAttemptElapsedMs)
			$passed = $false
			$syntheticFailureCount++
			$syntheticFailuresInjectedForRom++
			Write-Host "Injected synthetic probe failure on attempt $attempt for $rom" -ForegroundColor Yellow
		} else {
			$attemptStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
			& $runner -RomPath $rom -ExePath $ExePath -AutoStopTimeoutSeconds $AutoStopTimeoutSeconds -VerboseLaunch:$VerboseLaunch
			$attemptStopwatch.Stop()
			$lastAttemptElapsedMs = [int64]$attemptStopwatch.Elapsed.TotalMilliseconds
			$attemptElapsedMsValues.Add($lastAttemptElapsedMs)
			$exitCode = $LASTEXITCODE
			$passed = $exitCode -eq 0
		}
	}
	$romStopwatch.Stop()
	$elapsedMs = [int64]$romStopwatch.Elapsed.TotalMilliseconds
	$retryCountUsed = [Math]::Max(0, $attempt - 1)
	$attemptElapsedMsCsv = [string]::Join(';', ($attemptElapsedMsValues | ForEach-Object { $_.ToString() }))

	if (-not $passed) {
		$failed.Add($rom)
		$results.Add([PSCustomObject]@{
			RomPath = $rom
			Passed = $false
			ExitCode = $exitCode
			Attempts = $attempt
			ElapsedMs = $elapsedMs
			LastAttemptElapsedMs = $lastAttemptElapsedMs
			RetryCountUsed = $retryCountUsed
			AttemptElapsedCount = $attemptElapsedMsValues.Count
			AttemptElapsedMsCsv = $attemptElapsedMsCsv
			AttemptElapsedMs = @($attemptElapsedMsValues)
			SyntheticFailuresInjected = $syntheticFailuresInjectedForRom
		})
		Write-Host "FAILED: $rom" -ForegroundColor Red
		if ($StopOnFirstFailure) {
			Write-Host "StopOnFirstFailure is set; aborting matrix early." -ForegroundColor Red
			break
		}
	} else {
		$results.Add([PSCustomObject]@{
			RomPath = $rom
			Passed = $true
			ExitCode = 0
			Attempts = $attempt
			ElapsedMs = $elapsedMs
			LastAttemptElapsedMs = $lastAttemptElapsedMs
			RetryCountUsed = $retryCountUsed
			AttemptElapsedCount = $attemptElapsedMsValues.Count
			AttemptElapsedMsCsv = $attemptElapsedMsCsv
			AttemptElapsedMs = @($attemptElapsedMsValues)
			SyntheticFailuresInjected = $syntheticFailuresInjectedForRom
		})
		Write-Host "PASSED: $rom" -ForegroundColor Green
	}
}

$totalElapsedMs = [int64]0
$averageElapsedMs = [int64]0
$maxElapsedMs = [int64]0
$retriesUsedTotal = 0
$retryHitCount = 0
if ($results.Count -gt 0) {
	$totalElapsedMs = [int64](($results | Measure-Object -Property ElapsedMs -Sum).Sum)
	$averageElapsedMs = [int64][Math]::Round((($results | Measure-Object -Property ElapsedMs -Average).Average), 0)
	$maxElapsedMs = [int64](($results | Measure-Object -Property ElapsedMs -Maximum).Maximum)
	$retriesUsedTotal = [int](($results | Measure-Object -Property RetryCountUsed -Sum).Sum)
	$retryHitCount = [int](($results | Where-Object { $_.RetryCountUsed -gt 0 }).Count)
}

if (-not [string]::IsNullOrWhiteSpace($jsonOutputPath)) {
	$summaryObject = [PSCustomObject]@{
		TimestampUtc = [DateTime]::UtcNow.ToString("o")
		RunTimestamp = $runTimestamp
		ExePath = (Resolve-Path -LiteralPath $ExePath).Path
		AutoStopTimeoutSeconds = $AutoStopTimeoutSeconds
		Total = $results.Count
		Failed = $failed.Count
		TotalElapsedMs = $totalElapsedMs
		AverageElapsedMs = $averageElapsedMs
		MaxElapsedMs = $maxElapsedMs
		RetriesUsedTotal = $retriesUsedTotal
		RetryHitCount = $retryHitCount
		ForceFirstAttemptFailureProbeEnabled = [bool]$ForceFirstAttemptFailureProbe
		ForceProbeFailureCountPerRom = $ForceProbeFailureCountPerRom
		ForceProbeFailureEveryN = $ForceProbeFailureEveryN
		SyntheticFailureCount = $syntheticFailureCount
		CompactSummaryArtifactPath = $compactSummaryOutputPath
		CompactSummaryLatestAliasPath = $compactSummaryLatestAliasPath
		Results = $results
	}

	Ensure-ParentDirectory -TargetPath $jsonOutputPath

	$summaryObject | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $jsonOutputPath -Encoding utf8
	Write-Host "Summary JSON: $jsonOutputPath" -ForegroundColor Cyan
	Write-LatestAlias -ActualPath $jsonOutputPath -BasePath $SummaryJsonPath -Label "Summary JSON" -Enabled:$CreateLatestAliases
}

if (-not [string]::IsNullOrWhiteSpace($markdownOutputPath)) {
	Ensure-ParentDirectory -TargetPath $markdownOutputPath

	$lines = New-Object System.Collections.Generic.List[string]
	$lines.Add('# Genesis Smoke Matrix Report')
	$lines.Add('')
	$lines.Add("- Total: $($results.Count)")
	$lines.Add("- Passed: $($results.Count - $failed.Count)")
	$lines.Add("- Failed: $($failed.Count)")
	$lines.Add("- TotalElapsedMs: $totalElapsedMs")
	$lines.Add("- AverageElapsedMs: $averageElapsedMs")
	$lines.Add("- MaxElapsedMs: $maxElapsedMs")
	$lines.Add("- RetriesUsedTotal: $retriesUsedTotal")
	$lines.Add("- RetryHitCount: $retryHitCount")
	$lines.Add("- ForceFirstAttemptFailureProbeEnabled: $([bool]$ForceFirstAttemptFailureProbe)")
	$lines.Add("- SyntheticFailureCount: $syntheticFailureCount")
	$lines.Add("- ForceProbeFailureCountPerRom: $ForceProbeFailureCountPerRom")
	$lines.Add("- ForceProbeFailureEveryN: $ForceProbeFailureEveryN")
	if (-not [string]::IsNullOrWhiteSpace($compactSummaryOutputPath)) {
		$lines.Add("- CompactSummaryArtifactPath: $compactSummaryOutputPath")
	}
	if (-not [string]::IsNullOrWhiteSpace($compactSummaryLatestAliasPath)) {
		$lines.Add("- CompactSummaryLatestAliasPath: $compactSummaryLatestAliasPath")
	}
	$lines.Add('')
	$lines.Add('| ROM | Result | ExitCode | Attempts | RetryCountUsed | SyntheticFailuresInjected | AttemptElapsedCount | ElapsedMs | LastAttemptElapsedMs | AttemptElapsedMsCsv |')
	$lines.Add('|---|---:|---:|---:|---:|---:|---:|---:|---:|---|')
	foreach ($r in $results) {
		$resultText = if ($r.Passed) { 'PASS' } else { 'FAIL' }
		$lines.Add("| $($r.RomPath) | $resultText | $($r.ExitCode) | $($r.Attempts) | $($r.RetryCountUsed) | $($r.SyntheticFailuresInjected) | $($r.AttemptElapsedCount) | $($r.ElapsedMs) | $($r.LastAttemptElapsedMs) | $($r.AttemptElapsedMsCsv) |")
	}

	$lines | Set-Content -LiteralPath $markdownOutputPath -Encoding utf8
	Write-Host "Summary markdown: $markdownOutputPath" -ForegroundColor Cyan
	Write-LatestAlias -ActualPath $markdownOutputPath -BasePath $SummaryMarkdownPath -Label "Summary markdown" -Enabled:$CreateLatestAliases
}

if (-not [string]::IsNullOrWhiteSpace($csvOutputPath)) {
	Ensure-ParentDirectory -TargetPath $csvOutputPath

	$results | Export-Csv -LiteralPath $csvOutputPath -NoTypeInformation -Encoding utf8
	Write-Host "Summary CSV: $csvOutputPath" -ForegroundColor Cyan
	Write-LatestAlias -ActualPath $csvOutputPath -BasePath $SummaryCsvPath -Label "Summary CSV" -Enabled:$CreateLatestAliases
}

Write-Host "---" -ForegroundColor DarkGray
$ciSummaryLine = "CI_SUMMARY total=$($results.Count) passed=$($results.Count - $failed.Count) failed=$($failed.Count) retriesUsed=$retriesUsedTotal retryHitCount=$retryHitCount syntheticFailures=$syntheticFailureCount totalElapsedMs=$totalElapsedMs averageElapsedMs=$averageElapsedMs maxElapsedMs=$maxElapsedMs"

if (-not [string]::IsNullOrWhiteSpace($compactSummaryOutputPath)) {
	Ensure-ParentDirectory -TargetPath $compactSummaryOutputPath
	$ciSummaryLine | Set-Content -LiteralPath $compactSummaryOutputPath -Encoding utf8
	Write-Host "Compact summary artifact: $compactSummaryOutputPath" -ForegroundColor Cyan
	Write-LatestAlias -ActualPath $compactSummaryOutputPath -BasePath $CompactSummaryArtifactPath -Label "Compact summary artifact" -Enabled:$CreateLatestAliases
}

if ($failed.Count -gt 0) {
	Write-Host "Smoke matrix finished with failures ($($failed.Count))." -ForegroundColor Red
	$failed | ForEach-Object { Write-Host " - $_" -ForegroundColor Red }
	if ($CompactCiSummary) {
		Write-Host "$ciSummaryLine exitCode=1"
	}
	exit 1
}

Write-Host "Smoke matrix completed successfully." -ForegroundColor Green
if ($CompactCiSummary) {
	Write-Host "$ciSummaryLine exitCode=0"
}
exit 0
