param(
	[string]$BaselineDir = "artifacts/reference-validation-baseline",
	[string]$CandidateDir = "artifacts/reference-validation",
	[switch]$AllowDrift
)

$ErrorActionPreference = "Stop"

function Write-Section {
	param([string]$Title)
	Write-Host "`n=== $Title ==="
}

function Get-RequiredFile {
	param(
		[string]$Root,
		[string]$RelativePath
	)

	$path = Join-Path $Root $RelativePath
	if (-not (Test-Path $path)) {
		throw "Required file not found: $path"
	}
	return $path
}

function Get-SummaryLineMap {
	param([string]$FilePath)

	$map = @{}
	$summaryPattern = '^(HARNESS_SUMMARY|ROM_SET_SUMMARY|COMPAT_MATRIX_SUMMARY|PERF_GATE_SUMMARY|GEN_COMPAT_MATRIX_SUMMARY|GEN_PERF_GATE_SUMMARY|TIMING_SPIKE SUMMARY)'
	Get-Content $FilePath | ForEach-Object {
		$line = $_.Trim()
		if ($line -match $summaryPattern) {
			$key = $line.Split(' ')[0]
			if ($key -eq 'TIMING_SPIKE') {
				$key = 'TIMING_SPIKE_SUMMARY'
			}
			$map[$key] = $line
		}
	}
	return $map
}

function Compare-LineMaps {
	param(
		[hashtable]$Baseline,
		[hashtable]$Candidate,
		[string]$Label
	)

	$drift = @()
	$keys = ($Baseline.Keys + $Candidate.Keys | Sort-Object -Unique)
	foreach ($k in $keys) {
		if (-not $Baseline.ContainsKey($k)) {
			$drift += "[$Label] missing-baseline key=$k candidate='$($Candidate[$k])'"
			continue
		}
		if (-not $Candidate.ContainsKey($k)) {
			$drift += "[$Label] missing-candidate key=$k baseline='$($Baseline[$k])'"
			continue
		}
		if ($Baseline[$k] -ne $Candidate[$k]) {
			$drift += "[$Label] mismatch key=$k baseline='$($Baseline[$k])' candidate='$($Candidate[$k])'"
		}
	}
	return $drift
}

function Compare-Benchmarks {
	param(
		[string]$BaselinePath,
		[string]$CandidatePath
	)

	$drift = @()
	$baseline = Get-Content $BaselinePath -Raw | ConvertFrom-Json
	$candidate = Get-Content $CandidatePath -Raw | ConvertFrom-Json

	$baselineRows = @{}
	foreach ($row in $baseline.benchmarks) {
		$baselineRows[$row.name] = $row
	}

	foreach ($cand in $candidate.benchmarks) {
		if (-not $baselineRows.ContainsKey($cand.name)) {
			$drift += "[benchmarks] missing-baseline name=$($cand.name)"
			continue
		}

		$base = $baselineRows[$cand.name]
		if ($base.cpu_time -eq 0) {
			continue
		}
		$deltaPct = [math]::Round((($cand.cpu_time - $base.cpu_time) / $base.cpu_time) * 100.0, 2)
		if ([math]::Abs($deltaPct) -gt 5.0) {
			$drift += "[benchmarks] delta>5% name=$($cand.name) baseline=$($base.cpu_time) candidate=$($cand.cpu_time) delta=$deltaPct%"
		}
	}

	return $drift
}

Write-Section "Reference Validation Drift Comparison"

$baselineAtari = Get-RequiredFile -Root $BaselineDir -RelativePath "atari-harness.txt"
$baselineGenesis = Get-RequiredFile -Root $BaselineDir -RelativePath "genesis-harness.txt"
$baselineBench = Get-RequiredFile -Root $BaselineDir -RelativePath "atari-genesis-benchmarks.json"

$candidateAtari = Get-RequiredFile -Root $CandidateDir -RelativePath "atari-harness.txt"
$candidateGenesis = Get-RequiredFile -Root $CandidateDir -RelativePath "genesis-harness.txt"
$candidateBench = Get-RequiredFile -Root $CandidateDir -RelativePath "atari-genesis-benchmarks.json"

$drift = @()
$drift += Compare-LineMaps -Baseline (Get-SummaryLineMap -FilePath $baselineAtari) -Candidate (Get-SummaryLineMap -FilePath $candidateAtari) -Label "atari"
$drift += Compare-LineMaps -Baseline (Get-SummaryLineMap -FilePath $baselineGenesis) -Candidate (Get-SummaryLineMap -FilePath $candidateGenesis) -Label "genesis"
$drift += Compare-Benchmarks -BaselinePath $baselineBench -CandidatePath $candidateBench

if ($drift.Count -eq 0) {
	Write-Host "No drift detected."
	exit 0
}

Write-Section "Drift Report"
$drift | ForEach-Object { Write-Host $_ }

if (-not $AllowDrift) {
	exit 1
}

exit 0
