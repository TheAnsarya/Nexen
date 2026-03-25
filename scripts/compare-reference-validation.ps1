param(
	[string]$BaselineDir = "artifacts/reference-validation-baseline",
	[string]$CandidateDir = "artifacts/reference-validation",
	[string]$ReportPath = "",
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
			$digest = ""
			if ($line -match 'DIGEST=([0-9a-fA-F]+)') {
				$digest = $Matches[1].ToLowerInvariant()
			}
			$map[$key] = [ordered]@{
				line = $line
				digest = $digest
			}
		}
	}
	return $map
}

function Get-BenchmarkSchemaIssues {
	param(
		[object]$Artifact,
		[string]$Label
	)

	$issues = @()
	if ($null -eq $Artifact) {
		$issues += "[schema] artifact-null label=$Label"
		return $issues
	}

	if (-not ($Artifact.PSObject.Properties.Name -contains "context")) {
		$issues += "[schema] missing-field label=$Label field=context"
	} elseif ($null -eq $Artifact.context) {
		$issues += "[schema] null-field label=$Label field=context"
	} elseif (-not ($Artifact.context.PSObject.Properties.Name -contains "json_schema_version")) {
		$issues += "[schema] missing-field label=$Label field=context.json_schema_version"
	}

	if (-not ($Artifact.PSObject.Properties.Name -contains "benchmarks")) {
		$issues += "[schema] missing-field label=$Label field=benchmarks"
		return $issues
	}

	$rows = @($Artifact.benchmarks)
	if ($rows.Count -eq 0) {
		$issues += "[schema] empty-array label=$Label field=benchmarks"
		return $issues
	}

	$requiredFields = @("name", "run_name", "run_type", "repetitions", "threads", "iterations", "real_time", "cpu_time", "time_unit")
	$numericFields = @("repetitions", "threads", "iterations", "real_time", "cpu_time")

	for ($i = 0; $i -lt $rows.Count; $i++) {
		$row = $rows[$i]
		foreach ($field in $requiredFields) {
			if (-not ($row.PSObject.Properties.Name -contains $field)) {
				$issues += "[schema] missing-field label=$Label row=$i field=$field"
				continue
			}

			$value = $row.$field
			if ($null -eq $value) {
				$issues += "[schema] null-field label=$Label row=$i field=$field"
				continue
			}

			if ($field -eq "name" -or $field -eq "run_name" -or $field -eq "run_type" -or $field -eq "time_unit") {
				if ([string]::IsNullOrWhiteSpace([string]$value)) {
					$issues += "[schema] empty-string label=$Label row=$i field=$field"
				}
			}
		}

		foreach ($numericField in $numericFields) {
			if (-not ($row.PSObject.Properties.Name -contains $numericField)) {
				continue
			}

			$parsed = 0.0
			if (-not [double]::TryParse([string]$row.$numericField, [ref]$parsed)) {
				$issues += "[schema] non-numeric label=$Label row=$i field=$numericField value='$($row.$numericField)'"
			}
		}
	}

	return $issues
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
			$drift += "[$Label] missing-baseline key=$k candidate='$($Candidate[$k].line)'"
			continue
		}
		if (-not $Candidate.ContainsKey($k)) {
			$drift += "[$Label] missing-candidate key=$k baseline='$($Baseline[$k].line)'"
			continue
		}

		$baselineLine = $Baseline[$k].line
		$candidateLine = $Candidate[$k].line
		$baselineDigest = $Baseline[$k].digest
		$candidateDigest = $Candidate[$k].digest

		if (-not [string]::IsNullOrWhiteSpace($baselineDigest) -and -not [string]::IsNullOrWhiteSpace($candidateDigest) -and $baselineDigest -ne $candidateDigest) {
			$drift += "[$Label] digest-drift key=$k baseline=$baselineDigest candidate=$candidateDigest"
		}

		if ($baselineLine -ne $candidateLine) {
			$drift += "[$Label] summary-mismatch key=$k baseline='$baselineLine' candidate='$candidateLine'"
		}
	}
	return $drift
}

function Compare-Benchmarks {
	param(
		[string]$BaselinePath,
		[string]$CandidatePath
	)

	$result = [ordered]@{
		schemaIssues = @()
		drift = @()
	}

	$baseline = $null
	$candidate = $null

	try {
		$baseline = Get-Content $BaselinePath -Raw | ConvertFrom-Json
	} catch {
		$result.schemaIssues += "[schema] parse-failed label=baseline file=$BaselinePath"
	}

	try {
		$candidate = Get-Content $CandidatePath -Raw | ConvertFrom-Json
	} catch {
		$result.schemaIssues += "[schema] parse-failed label=candidate file=$CandidatePath"
	}

	if ($null -ne $baseline) {
		$result.schemaIssues += Get-BenchmarkSchemaIssues -Artifact $baseline -Label "baseline"
	}
	if ($null -ne $candidate) {
		$result.schemaIssues += Get-BenchmarkSchemaIssues -Artifact $candidate -Label "candidate"
	}

	if ($null -eq $baseline -or $null -eq $candidate) {
		return $result
	}

	$baselineRows = @{}
	foreach ($row in @($baseline.benchmarks)) {
		if ($null -eq $row.name) {
			continue
		}
		$baselineRows[[string]$row.name] = $row
	}

	$candidateRows = @{}
	foreach ($row in @($candidate.benchmarks)) {
		if ($null -eq $row.name) {
			continue
		}
		$candidateRows[[string]$row.name] = $row
	}

	$allNames = ($baselineRows.Keys + $candidateRows.Keys | Sort-Object -Unique)
	foreach ($name in $allNames) {
		if (-not $baselineRows.ContainsKey($name)) {
			$result.drift += "[benchmarks] missing-baseline name=$name"
			continue
		}
		if (-not $candidateRows.ContainsKey($name)) {
			$result.drift += "[benchmarks] missing-candidate name=$name"
			continue
		}

		$base = $baselineRows[$name]
		$cand = $candidateRows[$name]

		$baseCpu = 0.0
		$candCpu = 0.0
		if (-not [double]::TryParse([string]$base.cpu_time, [ref]$baseCpu) -or -not [double]::TryParse([string]$cand.cpu_time, [ref]$candCpu)) {
			$result.drift += "[benchmarks] non-numeric-cpu-time name=$name baseline='$($base.cpu_time)' candidate='$($cand.cpu_time)'"
			continue
		}

		if ($baseCpu -eq 0) {
			continue
		}

		$deltaPct = [math]::Round((($candCpu - $baseCpu) / $baseCpu) * 100.0, 2)
		if ([math]::Abs($deltaPct) -gt 5.0) {
			$result.drift += "[benchmarks] delta>5% name=$name baseline=$baseCpu candidate=$candCpu delta=$deltaPct%"
		}
	}

	return $result
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

$benchResult = Compare-Benchmarks -BaselinePath $baselineBench -CandidatePath $candidateBench
$schemaIssues = @($benchResult.schemaIssues)
$drift += @($benchResult.drift)

$report = [ordered]@{
	baselineDir = (Resolve-Path $BaselineDir).Path
	candidateDir = (Resolve-Path $CandidateDir).Path
	schemaIssues = $schemaIssues
	drift = $drift
	allowDrift = [bool]$AllowDrift
	status = if ($schemaIssues.Count -eq 0 -and $drift.Count -eq 0) { "pass" } else { "drift" }
}

if (-not [string]::IsNullOrWhiteSpace($ReportPath)) {
	$report | ConvertTo-Json -Depth 6 | Out-File -FilePath $ReportPath -Encoding utf8
}

if ($schemaIssues.Count -eq 0 -and $drift.Count -eq 0) {
	Write-Host "No drift detected."
	exit 0
}

if ($schemaIssues.Count -gt 0) {
	Write-Section "Schema Issues"
	$schemaIssues | ForEach-Object { Write-Host $_ }
}

Write-Section "Drift Report"
$drift | ForEach-Object { Write-Host $_ }

if (-not $AllowDrift) {
	exit 1
}

exit 0
