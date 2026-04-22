param(
	[Parameter(Mandatory = $true)]
	[string]$BaselineDir,
	[Parameter(Mandatory = $true)]
	[string]$CandidateDir,
	[string]$OutputReport = "~docs/testing/data/epic-21-visual-regression-report.md",
	[switch]$AllowDifferences
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

$baselinePath = Resolve-Path $BaselineDir -ErrorAction Stop
$candidatePath = Resolve-Path $CandidateDir -ErrorAction Stop
$reportPath = Join-Path $repoRoot $OutputReport
$reportDir = Split-Path -Parent $reportPath
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

function Get-FileHashMap([string]$rootDir) {
	$map = @{}
	Get-ChildItem -Path $rootDir -Recurse -File -Include *.png,*.jpg,*.jpeg | ForEach-Object {
		$relative = $_.FullName.Substring($rootDir.Length).TrimStart('\\')
		$hash = (Get-FileHash -Path $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
		$map[$relative] = $hash
	}
	return $map
}

$baselineFiles = Get-FileHashMap $baselinePath
$candidateFiles = Get-FileHashMap $candidatePath

$allKeys = @($baselineFiles.Keys + $candidateFiles.Keys | Sort-Object -Unique)
$missingInCandidate = @()
$newInCandidate = @()
$changed = @()
$unchanged = 0

foreach ($key in $allKeys) {
	$hasBaseline = $baselineFiles.ContainsKey($key)
	$hasCandidate = $candidateFiles.ContainsKey($key)

	if ($hasBaseline -and -not $hasCandidate) {
		$missingInCandidate += $key
		continue
	}

	if (-not $hasBaseline -and $hasCandidate) {
		$newInCandidate += $key
		continue
	}

	if ($baselineFiles[$key] -ne $candidateFiles[$key]) {
		$changed += $key
	} else {
		$unchanged++
	}
}

$hasDifferences = $missingInCandidate.Count -gt 0 -or $newInCandidate.Count -gt 0 -or $changed.Count -gt 0
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"

$lines = @()
$lines += "# Epic 21 Visual Regression Report"
$lines += ""
$lines += "- Generated: $timestamp"
$lines += "- Baseline: $baselinePath"
$lines += "- Candidate: $candidatePath"
$lines += "- Compared images: $($allKeys.Count)"
$lines += "- Unchanged images: $unchanged"
$lines += "- Changed images: $($changed.Count)"
$lines += "- Missing in candidate: $($missingInCandidate.Count)"
$lines += "- New in candidate: $($newInCandidate.Count)"
$lines += ""

if ($changed.Count -gt 0) {
	$lines += "## Changed Files"
	$lines += ""
	foreach ($item in $changed) {
		$lines += "- $item"
	}
	$lines += ""
}

if ($missingInCandidate.Count -gt 0) {
	$lines += "## Missing In Candidate"
	$lines += ""
	foreach ($item in $missingInCandidate) {
		$lines += "- $item"
	}
	$lines += ""
}

if ($newInCandidate.Count -gt 0) {
	$lines += "## New In Candidate"
	$lines += ""
	foreach ($item in $newInCandidate) {
		$lines += "- $item"
	}
	$lines += ""
}

if (-not $hasDifferences) {
	$lines += "## Result"
	$lines += ""
	$lines += "No visual differences detected between baseline and candidate screenshots."
	$lines += ""
}

$lines | Set-Content -Path $reportPath
Write-Host "Visual regression report written: $reportPath" -ForegroundColor Green

if ($hasDifferences -and -not $AllowDifferences) {
	Write-Error "Visual regression differences detected. Re-run with -AllowDifferences to report without failing."
	exit 1
}

exit 0
