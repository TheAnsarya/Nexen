param(
	[Parameter(Mandatory = $true)]
	[string[]]$RomPaths,
	[string]$ExePath = ".\bin\win-x64\Release\Nexen.exe",
	[int]$AutoStopTimeoutSeconds = 8,
	[string]$NonSonicRomPath,
	[string]$SummaryJsonPath
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

	& $runner -RomPath $rom -ExePath $ExePath -AutoStopTimeoutSeconds $AutoStopTimeoutSeconds
	if ($LASTEXITCODE -ne 0) {
		$failed.Add($rom)
		$results.Add([PSCustomObject]@{
			RomPath = $rom
			Passed = $false
			ExitCode = $LASTEXITCODE
		})
		Write-Host "FAILED: $rom" -ForegroundColor Red
	} else {
		$results.Add([PSCustomObject]@{
			RomPath = $rom
			Passed = $true
			ExitCode = 0
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

Write-Host "---" -ForegroundColor DarkGray
if ($failed.Count -gt 0) {
	Write-Host "Smoke matrix finished with failures ($($failed.Count))." -ForegroundColor Red
	$failed | ForEach-Object { Write-Host " - $_" -ForegroundColor Red }
	exit 1
}

Write-Host "Smoke matrix completed successfully." -ForegroundColor Green
exit 0
