param(
	[Parameter(Mandatory = $true)]
	[string[]]$RomPaths,
	[string]$ExePath = ".\bin\win-x64\Release\Nexen.exe",
	[int]$AutoStopTimeoutSeconds = 8,
	[string]$NonSonicRomPath,
	[string]$SummaryJsonPath,
	[string]$SummaryMarkdownPath,
	[string]$SummaryCsvPath,
	[int]$RetryCount = 0,
	[switch]$StopOnFirstFailure,
	[switch]$VerboseLaunch
)

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

Write-Host "Running Genesis Sonic smoke matrix for $($allRomPaths.Count) ROM(s)..." -ForegroundColor Cyan

foreach ($rom in $allRomPaths) {
	Write-Host "---" -ForegroundColor DarkGray
	Write-Host "ROM: $rom" -ForegroundColor Cyan

	$attempt = 0
	$passed = $false
	$exitCode = 0
	$maxAttempts = 1 + [Math]::Max(0, $RetryCount)
	while ($attempt -lt $maxAttempts -and -not $passed) {
		$attempt++
		if ($attempt -gt 1) {
			Write-Host "Retry $attempt/$maxAttempts for $rom" -ForegroundColor Yellow
		}

		& $runner -RomPath $rom -ExePath $ExePath -AutoStopTimeoutSeconds $AutoStopTimeoutSeconds -VerboseLaunch:$VerboseLaunch
		$exitCode = $LASTEXITCODE
		$passed = $exitCode -eq 0
	}

	if (-not $passed) {
		$failed.Add($rom)
		$results.Add([PSCustomObject]@{
			RomPath = $rom
			Passed = $false
			ExitCode = $exitCode
			Attempts = $attempt
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
		})
		Write-Host "PASSED: $rom" -ForegroundColor Green
	}
}

if (-not [string]::IsNullOrWhiteSpace($SummaryJsonPath)) {
	$summaryObject = [PSCustomObject]@{
		TimestampUtc = [DateTime]::UtcNow.ToString("o")
		ExePath = (Resolve-Path -LiteralPath $ExePath).Path
		AutoStopTimeoutSeconds = $AutoStopTimeoutSeconds
		Total = $results.Count
		Failed = $failed.Count
		Results = $results
	}

	$summaryDir = Split-Path -Path $SummaryJsonPath -Parent
	if (-not [string]::IsNullOrWhiteSpace($summaryDir)) {
		New-Item -ItemType Directory -Path $summaryDir -Force | Out-Null
	}

	$summaryObject | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $SummaryJsonPath -Encoding utf8
	Write-Host "Summary JSON: $SummaryJsonPath" -ForegroundColor Cyan
}

if (-not [string]::IsNullOrWhiteSpace($SummaryMarkdownPath)) {
	$mdDir = Split-Path -Path $SummaryMarkdownPath -Parent
	if (-not [string]::IsNullOrWhiteSpace($mdDir)) {
		New-Item -ItemType Directory -Path $mdDir -Force | Out-Null
	}

	$lines = New-Object System.Collections.Generic.List[string]
	$lines.Add('# Genesis Smoke Matrix Report')
	$lines.Add('')
	$lines.Add("- Total: $($results.Count)")
	$lines.Add("- Passed: $($results.Count - $failed.Count)")
	$lines.Add("- Failed: $($failed.Count)")
	$lines.Add('')
	$lines.Add('| ROM | Result | ExitCode | Attempts |')
	$lines.Add('|---|---:|---:|---:|')
	foreach ($r in $results) {
		$resultText = if ($r.Passed) { 'PASS' } else { 'FAIL' }
		$lines.Add("| $($r.RomPath) | $resultText | $($r.ExitCode) | $($r.Attempts) |")
	}

	$lines | Set-Content -LiteralPath $SummaryMarkdownPath -Encoding utf8
	Write-Host "Summary markdown: $SummaryMarkdownPath" -ForegroundColor Cyan
}

if (-not [string]::IsNullOrWhiteSpace($SummaryCsvPath)) {
	$csvDir = Split-Path -Path $SummaryCsvPath -Parent
	if (-not [string]::IsNullOrWhiteSpace($csvDir)) {
		New-Item -ItemType Directory -Path $csvDir -Force | Out-Null
	}

	$results | Export-Csv -LiteralPath $SummaryCsvPath -NoTypeInformation -Encoding utf8
	Write-Host "Summary CSV: $SummaryCsvPath" -ForegroundColor Cyan
}

Write-Host "---" -ForegroundColor DarkGray
if ($failed.Count -gt 0) {
	Write-Host "Smoke matrix finished with failures ($($failed.Count))." -ForegroundColor Red
	$failed | ForEach-Object { Write-Host " - $_" -ForegroundColor Red }
	exit 1
}

Write-Host "Smoke matrix completed successfully." -ForegroundColor Green
exit 0
