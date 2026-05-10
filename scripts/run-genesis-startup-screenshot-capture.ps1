param(
	[Parameter(Mandatory = $true)]
	[string]$RomPath,
	[string]$NexenTarget = ".\bin\win-x64\Release\Nexen.dll",
	[Alias("MesenTarget")]
	[string]$NexenRefTarget = "C:\Users\me\source\repos\Mesen2-Expanded\bin\win-x64\Debug\Mesen.dll",
	[Alias("AllowMissingMesenFrontend")]
	[switch]$AllowMissingNexenRefFrontend,
	[Alias("DisableMesenFallbackRunModes")]
	[switch]$DisableNexenRefFallbackRunModes,
	[string]$OutputDirectory = ".\reference\startup-logo-regression\screenshots",
	[int]$CaptureFrame = 180,
	[int]$TimeoutSeconds = 20,
	[int]$RetryCount = 3,
	[switch]$SuppressLegacyAliasNotice
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-LegacyAliasNotice {
	param(
		[switch]$SuppressNotice
	)

	if ($SuppressNotice) {
		return
	}

	$legacyAliases = @(
		"MesenTarget -> NexenRefTarget",
		"AllowMissingMesenFrontend -> AllowMissingNexenRefFrontend",
		"DisableMesenFallbackRunModes -> DisableNexenRefFallbackRunModes"
	)

	Write-Warning "Legacy Mesen* CLI aliases are accepted for compatibility. Prefer NexenRef* parameters in new automation calls."
	Write-Host ("Legacy aliases: {0}" -f ($legacyAliases -join "; ")) -ForegroundColor DarkYellow
}

Write-LegacyAliasNotice -SuppressNotice:$SuppressLegacyAliasNotice

function Resolve-OptionalPath {
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
	}
	catch {
		return $null
	}
}

function Resolve-FirstExistingPath {
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$CandidatePaths
	)

	foreach ($candidate in $CandidatePaths) {
		$resolved = Resolve-OptionalPath -Path $candidate
		if (-not [string]::IsNullOrWhiteSpace($resolved)) {
			return $resolved
		}
	}

	return $null
}

function Get-NexenRefTargetCandidates {
	param(
		[Parameter(Mandatory = $true)]
		[string]$ConfiguredPath,
		[switch]$DisableFallback
	)

	$candidates = New-Object System.Collections.Generic.List[string]
	$candidates.Add($ConfiguredPath)

	if (-not $DisableFallback) {
		$candidates.Add("C:\Users\me\source\repos\Mesen2-Expanded\bin\win-x64\Debug\Mesen.exe")
		$candidates.Add("C:\Users\me\source\repos\Mesen2-Expanded\bin\win-x64\Release\Mesen.exe")
		$candidates.Add("C:\Users\me\source\repos\Mesen2-Expanded\bin\win-x64\Debug\Mesen.dll")
		$candidates.Add("C:\Users\me\source\repos\Mesen2-Expanded\bin\win-x64\Release\Mesen.dll")
		$candidates.Add(".\Mesen2-Expanded\bin\win-x64\Debug\Mesen.exe")
		$candidates.Add(".\Mesen2-Expanded\bin\win-x64\Release\Mesen.exe")
		$candidates.Add(".\Mesen2-Expanded\bin\win-x64\Debug\Mesen.dll")
		$candidates.Add(".\Mesen2-Expanded\bin\win-x64\Release\Mesen.dll")
	}

	return $candidates
}

$repoRoot = (Resolve-Path ".").Path
$resolvedRom = (Resolve-Path $RomPath).Path
$resolvedNexenTarget = (Resolve-Path $NexenTarget).Path
$nexenRefCandidates = Get-NexenRefTargetCandidates -ConfiguredPath $NexenRefTarget -DisableFallback:$DisableNexenRefFallbackRunModes
$resolvedNexenRefTarget = Resolve-FirstExistingPath -CandidatePaths $nexenRefCandidates
$resolvedOutputDirectory = Join-Path $repoRoot $OutputDirectory
$luaScript = Join-Path $repoRoot "scripts\capture-genesis-startup-screenshot.lua"

if (-not (Test-Path $luaScript)) {
	throw "Lua script not found: $luaScript"
}

New-Item -ItemType Directory -Path $resolvedOutputDirectory -Force | Out-Null

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$nexenOut = Join-Path $resolvedOutputDirectory "nexen-startup-frame-$CaptureFrame-$timestamp.png"
$nexenRefOut = Join-Path $resolvedOutputDirectory "nexenref-startup-frame-$CaptureFrame-$timestamp.png"

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

function Invoke-CaptureRunWithRetry {
	param(
		[Parameter(Mandatory = $true)]
		[string]$TargetPath,
		[Parameter(Mandatory = $true)]
		[string]$OutputPath,
		[Parameter(Mandatory = $true)]
		[string]$Label
	)

	$lastError = $null
	for ($attempt = 1; $attempt -le $RetryCount; $attempt++) {
		try {
			Invoke-CaptureRun -TargetPath $TargetPath -OutputPath $OutputPath -Label $Label
			return
		}
		catch {
			$lastError = $_
			if ($attempt -lt $RetryCount) {
				Write-Host "$Label capture attempt $attempt failed, retrying..." -ForegroundColor Yellow
			}
		}
	}

	throw "$Label capture failed after $RetryCount attempts. Last error: $lastError"
}

$oldPath = $env:NEXEN_STARTUP_SCREENSHOT_PATH
$oldFrame = $env:NEXEN_STARTUP_SCREENSHOT_FRAME
$nexenRefSkipped = $false
$nexenRefSkipReason = ""

try {
	if ([string]::IsNullOrWhiteSpace($resolvedNexenRefTarget)) {
		if ($AllowMissingNexenRefFrontend) {
			$nexenRefSkipped = $true
			$nexenRefSkipReason = "NexenRef target not found"
			Write-Warning "Skipping NexenRef screenshot capture: $nexenRefSkipReason"
		}
		else {
			throw "Unable to resolve NexenRef target. Provide -NexenRefTarget or use -AllowMissingNexenRefFrontend."
		}
	}
	else {
		try {
			Invoke-CaptureRunWithRetry -TargetPath $resolvedNexenRefTarget -OutputPath $nexenRefOut -Label "NexenRef"
		}
		catch {
			if ($AllowMissingNexenRefFrontend) {
				$nexenRefSkipped = $true
				$nexenRefSkipReason = "NexenRef launch failed: $($_.Exception.Message)"
				Write-Warning "Skipping NexenRef screenshot capture: $nexenRefSkipReason"
			}
			else {
				throw
			}
		}
	}

	Invoke-CaptureRunWithRetry -TargetPath $resolvedNexenTarget -OutputPath $nexenOut -Label "Nexen"
}
finally {
	$env:NEXEN_STARTUP_SCREENSHOT_PATH = $oldPath
	$env:NEXEN_STARTUP_SCREENSHOT_FRAME = $oldFrame
}

$summary = [ordered]@{
	timestamp = $timestamp
	captureFrame = $CaptureFrame
	rom = "$resolvedRom"
	nexenRefTargetResolved = if ([string]::IsNullOrWhiteSpace($resolvedNexenRefTarget)) { $null } else { "$resolvedNexenRefTarget" }
	nexenRefCaptureSkipped = $nexenRefSkipped
	nexenRefCaptureSkipReason = if ([string]::IsNullOrWhiteSpace($nexenRefSkipReason)) { $null } else { $nexenRefSkipReason }
	nexenRefScreenshot = if ($nexenRefSkipped) { $null } else { "$nexenRefOut" }
	nexenScreenshot = "$nexenOut"
}

$summaryPath = Join-Path $resolvedOutputDirectory "startup-screenshot-summary-$timestamp.json"
$latestPath = Join-Path $resolvedOutputDirectory "startup-screenshot-summary.latest.json"
$summaryJson = $summary | ConvertTo-Json -Depth 4
Set-Content -Path $summaryPath -Value $summaryJson -Encoding UTF8
Set-Content -Path $latestPath -Value $summaryJson -Encoding UTF8

if ($nexenRefSkipped) {
	Write-Host "NexenRef screenshot: skipped ($nexenRefSkipReason)" -ForegroundColor Yellow
}
else {
	Write-Host "NexenRef screenshot: $nexenRefOut" -ForegroundColor Green
}
Write-Host "Nexen screenshot: $nexenOut" -ForegroundColor Green
Write-Host "Summary: $summaryPath" -ForegroundColor Green



