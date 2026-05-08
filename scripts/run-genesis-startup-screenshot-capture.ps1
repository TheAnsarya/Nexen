param(
	[Parameter(Mandatory = $true)]
	[string]$RomPath,
	[string]$NexenTarget = ".\bin\win-x64\Release\Nexen.dll",
	[string]$MesenTarget = "C:\Users\me\source\repos\Mesen2-Expanded\bin\win-x64\Debug\Mesen.dll",
	[string]$OutputDirectory = ".\reference\startup-logo-regression\screenshots",
	[int]$CaptureFrame = 180,
	[int]$TimeoutSeconds = 20
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path ".").Path
$resolvedRom = (Resolve-Path $RomPath).Path
$resolvedNexenTarget = (Resolve-Path $NexenTarget).Path
$resolvedMesenTarget = (Resolve-Path $MesenTarget).Path
$resolvedOutputDirectory = Join-Path $repoRoot $OutputDirectory
$luaScript = Join-Path $repoRoot "scripts\capture-genesis-startup-screenshot.lua"

if (-not (Test-Path $luaScript)) {
	throw "Lua script not found: $luaScript"
}

New-Item -ItemType Directory -Path $resolvedOutputDirectory -Force | Out-Null

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$nexenOut = Join-Path $resolvedOutputDirectory "nexen-startup-frame-$CaptureFrame-$timestamp.png"
$mesenOut = Join-Path $resolvedOutputDirectory "mesen-startup-frame-$CaptureFrame-$timestamp.png"

function Invoke-CaptureRun {
	param(
		[Parameter(Mandatory = $true)]
		[string]$TargetPath,
		[Parameter(Mandatory = $true)]
		[string]$OutputPath,
		[Parameter(Mandatory = $true)]
		[string]$Label
	)

	$env:NEXEN_STARTUP_SCREENSHOT_PATH = $OutputPath
	$env:NEXEN_STARTUP_SCREENSHOT_FRAME = "$CaptureFrame"

	$commonArgs = @(
		"--testRunner",
		"--timeout=$TimeoutSeconds",
		"--Debug.ScriptWindow.AllowIoOsAccess=true",
		"--Debug.ScriptWindow.AllowNetworkAccess=false",
		$luaScript,
		$resolvedRom
	)

	$runnerPath = $TargetPath
	$args = @()
	if ([System.IO.Path]::GetExtension($TargetPath).ToLowerInvariant() -eq ".dll") {
		$runnerPath = "dotnet"
		$args += $TargetPath
	}
	$args += $commonArgs

	Write-Host "Capturing $Label screenshot..." -ForegroundColor Cyan
	& $runnerPath @args
	$exitCode = $LASTEXITCODE
	if ($exitCode -ne 0) {
		throw "$Label capture failed with exit code $exitCode"
	}
	if (-not (Test-Path $OutputPath)) {
		throw "$Label capture did not produce output: $OutputPath"
	}
}

$oldPath = $env:NEXEN_STARTUP_SCREENSHOT_PATH
$oldFrame = $env:NEXEN_STARTUP_SCREENSHOT_FRAME

try {
	Invoke-CaptureRun -TargetPath $resolvedMesenTarget -OutputPath $mesenOut -Label "Mesen2-Expanded"
	Invoke-CaptureRun -TargetPath $resolvedNexenTarget -OutputPath $nexenOut -Label "Nexen"
}
finally {
	$env:NEXEN_STARTUP_SCREENSHOT_PATH = $oldPath
	$env:NEXEN_STARTUP_SCREENSHOT_FRAME = $oldFrame
}

$summary = [ordered]@{
	timestamp = $timestamp
	captureFrame = $CaptureFrame
	rom = "$resolvedRom"
	mesenScreenshot = "$mesenOut"
	nexenScreenshot = "$nexenOut"
}

$summaryPath = Join-Path $resolvedOutputDirectory "startup-screenshot-summary-$timestamp.json"
$latestPath = Join-Path $resolvedOutputDirectory "startup-screenshot-summary.latest.json"
$summaryJson = $summary | ConvertTo-Json -Depth 4
Set-Content -Path $summaryPath -Value $summaryJson -Encoding UTF8
Set-Content -Path $latestPath -Value $summaryJson -Encoding UTF8

Write-Host "Mesen screenshot: $mesenOut" -ForegroundColor Green
Write-Host "Nexen screenshot: $nexenOut" -ForegroundColor Green
Write-Host "Summary: $summaryPath" -ForegroundColor Green
