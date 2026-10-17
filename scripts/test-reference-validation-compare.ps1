$ErrorActionPreference = "Stop"

function Assert-True {
	param(
		[bool]$Condition,
		[string]$Message
	)

	if (-not $Condition) {
		throw "Assert-True failed: $Message"
	}
}

function Assert-Equal {
	param(
		[object]$Actual,
		[object]$Expected,
		[string]$Message
	)

	if ($Actual -ne $Expected) {
		throw "Assert-Equal failed: $Message (expected='$Expected' actual='$Actual')"
	}
}

function Assert-Contains {
	param(
		[string]$Haystack,
		[string]$Needle,
		[string]$Message
	)

	if (-not $Haystack.Contains($Needle)) {
		throw "Assert-Contains failed: $Message (needle='$Needle')"
	}
}

function New-ValidationArtifactPack {
	param(
		[string]$Root,
		[string]$AtariDigest,
		[string]$GenesisDigest,
		[double]$CpuTime,
		[switch]$DropCpuTime
	)

	New-Item -ItemType Directory -Force -Path $Root | Out-Null

	$atariLines = @(
		"HARNESS_SUMMARY PASS=4 FAIL=0 DIGEST=$AtariDigest",
		"ROM_SET_SUMMARY PASS=1 FAIL=0 DIGEST=$AtariDigest",
		"COMPAT_MATRIX_SUMMARY PASS=1 FAIL=0 DIGEST=$AtariDigest",
		"PERF_GATE_SUMMARY PASS=1 FAIL=0 BUDGET_US=25000 DIGEST=$AtariDigest",
		"TIMING_SPIKE SUMMARY STABLE=true DIGEST=$AtariDigest"
	)
	$genesisLines = @(
		"GEN_COMPAT_MATRIX_SUMMARY PASS=1 FAIL=0 DIGEST=$GenesisDigest",
		"GEN_PERF_GATE_SUMMARY PASS=1 FAIL=0 BUDGET_US=25000 DIGEST=$GenesisDigest"
	)

	$benchRow = [ordered]@{
		name = "BM_Atari2600_Sample"
		run_name = "BM_Atari2600_Sample"
		run_type = "iteration"
		repetitions = 3
		threads = 1
		iterations = 100
		real_time = $CpuTime
		cpu_time = $CpuTime
		time_unit = "ns"
	}

	if ($DropCpuTime) {
		$benchRow.Remove("cpu_time") | Out-Null
	}

	$benchmarkArtifact = [ordered]@{
		context = [ordered]@{
			json_schema_version = 1
		}
		benchmarks = @($benchRow)
	}

	$atariPath = Join-Path $Root "atari-harness.txt"
	$genesisPath = Join-Path $Root "genesis-harness.txt"
	$benchPath = Join-Path $Root "atari-genesis-benchmarks.json"

	$atariLines | Out-File -FilePath $atariPath -Encoding utf8
	$genesisLines | Out-File -FilePath $genesisPath -Encoding utf8
	$benchmarkArtifact | ConvertTo-Json -Depth 5 | Out-File -FilePath $benchPath -Encoding utf8
}

function Invoke-CompareScript {
	param(
		[string]$ScriptPath,
		[string]$BaselineDir,
		[string]$CandidateDir,
		[switch]$AllowDrift,
		[string]$ReportPath = ""
	)

	$engine = if (Get-Command pwsh -ErrorAction SilentlyContinue) { "pwsh" } else { "powershell" }
	$args = @("-NoProfile", "-File", $ScriptPath, "-BaselineDir", $BaselineDir, "-CandidateDir", $CandidateDir)
	if ($AllowDrift) {
		$args += "-AllowDrift"
	}
	if (-not [string]::IsNullOrWhiteSpace($ReportPath)) {
		$args += @("-ReportPath", $ReportPath)
	}

	$output = & $engine @args 2>&1
	return [ordered]@{
		output = @($output)
		exitCode = $LASTEXITCODE
	}
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$compareScript = Join-Path $scriptDir "compare-reference-validation.ps1"
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("nexen-compare-reference-validation-" + [System.Guid]::NewGuid().ToString("N"))

try {
	$baselineDir = Join-Path $testRoot "baseline"
	$candidateDir = Join-Path $testRoot "candidate"
	$reportPath = Join-Path $testRoot "drift-report.json"

	New-ValidationArtifactPack -Root $baselineDir -AtariDigest "1111111111111111" -GenesisDigest "2222222222222222" -CpuTime 10.0
	New-ValidationArtifactPack -Root $candidateDir -AtariDigest "1111111111111111" -GenesisDigest "2222222222222222" -CpuTime 10.0

	$stableRunA = Invoke-CompareScript -ScriptPath $compareScript -BaselineDir $baselineDir -CandidateDir $candidateDir
	$stableRunB = Invoke-CompareScript -ScriptPath $compareScript -BaselineDir $baselineDir -CandidateDir $candidateDir
	$stableOutputA = ($stableRunA.output -join "`n")
	$stableOutputB = ($stableRunB.output -join "`n")
	Assert-Equal -Actual $stableRunA.exitCode -Expected 0 -Message "stable comparison should pass"
	Assert-Equal -Actual $stableRunB.exitCode -Expected 0 -Message "stable comparison should pass repeatedly"
	Assert-Contains -Haystack $stableOutputA -Needle "No drift detected." -Message "stable run should report no drift"
	Assert-Equal -Actual $stableOutputA -Expected $stableOutputB -Message "stable output should be deterministic"

	New-ValidationArtifactPack -Root $candidateDir -AtariDigest "aaaaaaaaaaaaaaaa" -GenesisDigest "2222222222222222" -CpuTime 10.0
	$driftRun = Invoke-CompareScript -ScriptPath $compareScript -BaselineDir $baselineDir -CandidateDir $candidateDir
	$driftOutput = ($driftRun.output -join "`n")
	Assert-Equal -Actual $driftRun.exitCode -Expected 1 -Message "digest drift should fail"
	Assert-Contains -Haystack $driftOutput -Needle "[atari] digest-drift key=HARNESS_SUMMARY" -Message "digest drift line should be emitted"
	Assert-Contains -Haystack $driftOutput -Needle "=== Drift Report ===" -Message "drift section should be emitted"

	$allowDriftRun = Invoke-CompareScript -ScriptPath $compareScript -BaselineDir $baselineDir -CandidateDir $candidateDir -AllowDrift -ReportPath $reportPath
	Assert-Equal -Actual $allowDriftRun.exitCode -Expected 0 -Message "allow drift should return success"
	Assert-True -Condition (Test-Path $reportPath) -Message "report file should be written"

	$report = Get-Content $reportPath -Raw | ConvertFrom-Json
	Assert-Equal -Actual $report.status -Expected "drift" -Message "report status should indicate drift"
	Assert-True -Condition ($report.drift.Count -ge 1) -Message "report should include drift rows"

	New-ValidationArtifactPack -Root $candidateDir -AtariDigest "1111111111111111" -GenesisDigest "2222222222222222" -CpuTime 10.0 -DropCpuTime
	$schemaRun = Invoke-CompareScript -ScriptPath $compareScript -BaselineDir $baselineDir -CandidateDir $candidateDir
	$schemaOutput = ($schemaRun.output -join "`n")
	Assert-Equal -Actual $schemaRun.exitCode -Expected 1 -Message "schema validation failures should fail"
	Assert-Contains -Haystack $schemaOutput -Needle "[schema] missing-field label=candidate row=0 field=cpu_time" -Message "schema issue should identify missing cpu_time"
	Assert-Contains -Haystack $schemaOutput -Needle "=== Schema Issues ===" -Message "schema section should be emitted"

	Write-Host "Reference validation compare script tests passed."
	exit 0
} finally {
	if (Test-Path $testRoot) {
		Remove-Item -Path $testRoot -Recurse -Force
	}
}
