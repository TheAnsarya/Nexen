param(
	[string]$Configuration = "Release",
	[string]$OutputFolder = "~docs/testing/data",
	[string]$Label = "baseline",
	[switch]$SkipBenchmarks
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

$outputDir = Join-Path $repoRoot $OutputFolder
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

$timestamp = Get-Date -Format "yyyy-MM-ddTHH:mm:ssK"
$dateStamp = Get-Date -Format "yyyy-MM-dd"
$safeLabel = $Label.ToLowerInvariant().Replace(" ", "-")
$baselinePath = Join-Path $outputDir ("epic-21-ui-{0}-{1}.json" -f $safeLabel, $dateStamp)

$metricsPath = Join-Path $env:USERPROFILE "Documents\Nexen\setup-wizard-metrics.json"
$metrics = [ordered]@{
	LaunchCount = 0
	ResumeLaunchCount = 0
	CompletionCount = 0
	CancelCount = 0
	StartFreshCount = 0
	ProfileToggleCount = 0
	StorageToggleCount = 0
	CustomizeToggleCount = 0
	TotalBacktrackCount = 0
}

if (Test-Path $metricsPath) {
	$raw = Get-Content $metricsPath -Raw | ConvertFrom-Json
	$metrics = [ordered]@{
		LaunchCount = [int]$raw.LaunchCount
		ResumeLaunchCount = [int]$raw.ResumeLaunchCount
		CompletionCount = [int]$raw.CompletionCount
		CancelCount = [int]$raw.CancelCount
		StartFreshCount = [int]$raw.StartFreshCount
		ProfileToggleCount = [int]$raw.ProfileToggleCount
		StorageToggleCount = [int]$raw.StorageToggleCount
		CustomizeToggleCount = [int]$raw.CustomizeToggleCount
		TotalBacktrackCount = [int]$raw.TotalBacktrackCount
	}
}

$launchCount = [double]$metrics.LaunchCount
$outcomeCount = [double]($metrics.CompletionCount + $metrics.CancelCount)
$interactionCount = [double]($metrics.ProfileToggleCount + $metrics.StorageToggleCount + $metrics.CustomizeToggleCount)

$derived = [ordered]@{
	CompletionRate = if ($launchCount -gt 0) { [Math]::Round($metrics.CompletionCount / $launchCount, 4) } else { 0.0 }
	ResumeRate = if ($launchCount -gt 0) { [Math]::Round($metrics.ResumeLaunchCount / $launchCount, 4) } else { 0.0 }
	BacktracksPerOutcome = if ($outcomeCount -gt 0) { [Math]::Round($metrics.TotalBacktrackCount / $outcomeCount, 4) } else { 0.0 }
	InteractionTogglesPerLaunch = if ($launchCount -gt 0) { [Math]::Round($interactionCount / $launchCount, 4) } else { 0.0 }
}

$benchmarkCommand = @(
	"dotnet", "run", "--project", "Benchmarks/Nexen.Benchmarks.csproj", "-c", $Configuration, "--",
	"--filter", "*TasUiBenchmarks.Selection_ContiguousRange*", "*TasUiBenchmarks.Search_ButtonState*",
	"--job", "short"
)

$benchmarkReports = @()
$benchmarkExitCode = $null

if (-not $SkipBenchmarks) {
	Write-Host "Running focused Epic 21 UI benchmark scenarios..." -ForegroundColor Cyan
	$benchStartUtc = [DateTime]::UtcNow
	& $benchmarkCommand[0] $benchmarkCommand[1..($benchmarkCommand.Length - 1)]
	$benchmarkExitCode = $LASTEXITCODE
	if ($benchmarkExitCode -ne 0) {
		throw "Benchmark execution failed with exit code $benchmarkExitCode"
	}

	$resultsDir = Join-Path $repoRoot "BenchmarkDotNet.Artifacts/results"
	if (Test-Path $resultsDir) {
		$benchmarkReports = Get-ChildItem -Path $resultsDir -File |
			Where-Object { $_.LastWriteTimeUtc -ge $benchStartUtc -and $_.Name -like "*report*" } |
			Sort-Object LastWriteTime -Descending |
			Select-Object -First 6 |
			ForEach-Object { $_.FullName.Replace($repoRoot, "").TrimStart('\\') }
	}
}

$payload = [ordered]@{
	Issue = "#1414"
	CapturedAt = $timestamp
	Label = $Label
	MetricsFile = $metricsPath
	SetupWizardMetrics = $metrics
	DerivedUxMetrics = $derived
	RegressionThresholds = [ordered]@{
		MinCompletionRate = 0.70
		MaxBacktracksPerOutcome = 1.50
		MaxInteractionTogglesPerLaunch = 3.00
	}
	Benchmark = [ordered]@{
		Skipped = [bool]$SkipBenchmarks
		Command = ($benchmarkCommand -join " ")
		Reports = $benchmarkReports
	}
}

$payload | ConvertTo-Json -Depth 8 | Set-Content -Path $baselinePath
Write-Host "Epic 21 baseline snapshot saved: $baselinePath" -ForegroundColor Green
