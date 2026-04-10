param(
	[string]$Configuration = "Release",
	[string]$Platform = "x64",
	[int]$Repetitions = 12,
	[string]$Label = "current",
	[string]$OutputRoot = "BenchmarkDotNet.Artifacts/results/hotpath/memory-manager"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

$outputDir = Join-Path $repoRoot $OutputRoot
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$outFile = Join-Path $outputDir ("memhotpath-{0}-{1}.json" -f $Label, $timestamp)

$benchFilter = "BM_(GbaMemMgr_ProcessDma_NoPending|GbaMemMgr_ProcessInternalCycle_Pending(0Pct|5Pct|50Pct)|NesMapper_ReadVram_CommonPath)"
$benchExe = Join-Path $repoRoot ("bin/win-{0}/{1}/Core.Benchmarks.exe" -f $Platform, $Configuration)

if (-not (Test-Path $benchExe)) {
	throw "Core.Benchmarks executable not found at: $benchExe"
}

Write-Host "Running memory-manager hotpath benchmark suite..." -ForegroundColor Cyan
Write-Host "Output: $outFile" -ForegroundColor Cyan

& $benchExe `
	--benchmark_filter="$benchFilter" `
	--benchmark_repetitions=$Repetitions `
	--benchmark_report_aggregates_only=true `
	--benchmark_out="$outFile" `
	--benchmark_out_format=json

Write-Host "Completed. Results written to: $outFile" -ForegroundColor Green
