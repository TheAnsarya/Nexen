param(
	[Parameter(Mandatory = $true)]
	[string]$RomPath,
	[string]$ExePath = ".\bin\win-x64\Release\Nexen.exe",
	[int]$AutoStopTimeoutSeconds = 8
)

$resolvedExe = (Resolve-Path -LiteralPath $ExePath -ErrorAction Stop).Path
$resolvedRom = (Resolve-Path -LiteralPath $RomPath -ErrorAction Stop).Path

Write-Host "Launching Nexen smoke run" -ForegroundColor Cyan
Write-Host "Exe: $resolvedExe"
Write-Host "Rom: $resolvedRom"

$process = Start-Process -FilePath $resolvedExe -ArgumentList @($resolvedRom) -PassThru

if ($null -eq $process) {
	Write-Error "Failed to launch Nexen process."
	exit 1
}

Write-Host "Started PID=$($process.Id)" -ForegroundColor Green

$running = Get-Process -Id $process.Id -ErrorAction SilentlyContinue
if ($null -eq $running) {
	Write-Error "Process terminated immediately after launch."
	exit 2
}

if ($AutoStopTimeoutSeconds -gt 0) {
	try {
		Wait-Process -Id $process.Id -Timeout $AutoStopTimeoutSeconds -ErrorAction Stop
		Write-Host "Process exited before timeout." -ForegroundColor Yellow
	} catch {
		Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
		Write-Host "Auto-stopped PID=$($process.Id) after timeout $AutoStopTimeoutSeconds s." -ForegroundColor Yellow
	}
}

Write-Host "Smoke run completed successfully." -ForegroundColor Green
exit 0
