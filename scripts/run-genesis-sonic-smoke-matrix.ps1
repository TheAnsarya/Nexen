param(
	[Parameter(Mandatory = $true)]
	[string[]]$RomPaths,
	[string]$ExePath = ".\bin\win-x64\Release\Nexen.exe",
	[int]$AutoStopTimeoutSeconds = 8
)

$runner = Join-Path $PSScriptRoot "run-genesis-sonic-smoke.ps1"
if (-not (Test-Path -LiteralPath $runner)) {
	Write-Error "Missing smoke runner script: $runner"
	exit 10
}

$failed = New-Object System.Collections.Generic.List[string]

Write-Host "Running Genesis Sonic smoke matrix for $($RomPaths.Count) ROM(s)..." -ForegroundColor Cyan

foreach ($rom in $RomPaths) {
	Write-Host "---" -ForegroundColor DarkGray
	Write-Host "ROM: $rom" -ForegroundColor Cyan

	& $runner -RomPath $rom -ExePath $ExePath -AutoStopTimeoutSeconds $AutoStopTimeoutSeconds
	if ($LASTEXITCODE -ne 0) {
		$failed.Add($rom)
		Write-Host "FAILED: $rom" -ForegroundColor Red
	} else {
		Write-Host "PASSED: $rom" -ForegroundColor Green
	}
}

Write-Host "---" -ForegroundColor DarkGray
if ($failed.Count -gt 0) {
	Write-Host "Smoke matrix finished with failures ($($failed.Count))." -ForegroundColor Red
	$failed | ForEach-Object { Write-Host " - $_" -ForegroundColor Red }
	exit 1
}

Write-Host "Smoke matrix completed successfully." -ForegroundColor Green
exit 0
