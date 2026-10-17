param(
	[string]$OutputDir = "artifacts/reference-validation",
	[switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Write-Header {
	param([string]$Message)
	Write-Host "`n=== $Message ==="
}

Write-Header "Reference Validation Artifact Pack"

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

if (-not $SkipBuild) {
	Write-Header "Build Release x64"
	& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$testsExe = Join-Path $repoRoot "bin\win-x64\Release\Core.Tests.exe"
$benchExe = Join-Path $repoRoot "bin\win-x64\Release\Core.Benchmarks.exe"

if (-not (Test-Path $testsExe)) {
	throw "Core.Tests.exe not found at $testsExe"
}

if (-not (Test-Path $benchExe)) {
	throw "Core.Benchmarks.exe not found at $benchExe"
}

$atariOut = Join-Path $OutputDir "atari-harness.txt"
$genesisOut = Join-Path $OutputDir "genesis-harness.txt"
$benchOut = Join-Path $OutputDir "atari-genesis-benchmarks.json"
$summaryOut = Join-Path $OutputDir "reference-validation-summary.json"

Write-Header "Run Atari harness-focused tests"
& $testsExe --gtest_filter="Atari2600TimingSpikeHarnessTests.*:Atari2600CompatibilityMatrixTests.*:Atari2600PerformanceGateTests.*" --gtest_brief=1 *>&1 | Out-File -FilePath $atariOut -Encoding utf8
$atariExit = $LASTEXITCODE

Write-Header "Run Genesis harness-focused tests"
& $testsExe --gtest_filter="GenesisCompatibilityHarnessTests.*:GenesisPerformanceGateTests.*:GenesisCombinedInteractionTests.*" --gtest_brief=1 *>&1 | Out-File -FilePath $genesisOut -Encoding utf8
$genesisExit = $LASTEXITCODE

Write-Header "Run focused Atari/Genesis benchmarks"
& $benchExe --benchmark_filter="BM_(Atari2600|Genesis)" --benchmark_repetitions=3 --benchmark_report_aggregates_only=true --benchmark_out=$benchOut --benchmark_out_format=json
$benchExit = $LASTEXITCODE

$summary = [ordered]@{
	runAt = (Get-Date).ToString("o")
	outputDir = (Resolve-Path $OutputDir).Path
	artifacts = [ordered]@{
		atariHarness = [ordered]@{ path = $atariOut; exitCode = $atariExit }
		genesisHarness = [ordered]@{ path = $genesisOut; exitCode = $genesisExit }
		benchmarks = [ordered]@{ path = $benchOut; exitCode = $benchExit }
	}
	notes = @(
		"This pack is for consistent regression comparison, not a full ROM set runner.",
		"Use alongside the example manifest in ~docs/testing/reference-rom-validation-manifest.example.json."
	)
}

$summary | ConvertTo-Json -Depth 5 | Out-File -FilePath $summaryOut -Encoding utf8

Write-Header "Done"
Write-Host "Artifacts written to: $OutputDir"
Write-Host "Summary file: $summaryOut"
Write-Host "Exit codes: Atari=$atariExit Genesis=$genesisExit Bench=$benchExit"

if ($atariExit -ne 0 -or $genesisExit -ne 0 -or $benchExit -ne 0) {
	exit 1
}

exit 0
