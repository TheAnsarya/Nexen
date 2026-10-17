param(
	[switch]$CompactCiSummary
)

$matrixScript = Join-Path $PSScriptRoot "run-genesis-sonic-smoke-matrix.ps1"
if (-not (Test-Path -LiteralPath $matrixScript)) {
	Write-Error "Missing matrix script: $matrixScript"
	exit 20
}

Write-Host "Running compatibility self-test (default mode)..." -ForegroundColor Cyan
$defaultArgs = @('-ExecutionPolicy', 'Bypass', '-File', $matrixScript, '-CompatibilitySelfTest')
if ($CompactCiSummary) {
	$defaultArgs += '-CompactCiSummary'
}
powershell @defaultArgs
if ($LASTEXITCODE -ne 0) {
	Write-Error "Compatibility self-test failed in default mode."
	exit 21
}

Write-Host "Running compatibility self-test (forced legacy fallback mode)..." -ForegroundColor Cyan
$fallbackArgs = @('-ExecutionPolicy', 'Bypass', '-File', $matrixScript, '-CompatibilitySelfTest', '-ForceLegacyHashFallback')
if ($CompactCiSummary) {
	$fallbackArgs += '-CompactCiSummary'
}
powershell @fallbackArgs
if ($LASTEXITCODE -ne 0) {
	Write-Error "Compatibility self-test failed in forced fallback mode."
	exit 22
}

Write-Host "Genesis smoke compatibility self-tests passed." -ForegroundColor Green
if ($CompactCiSummary) {
	Write-Host "CI_SUMMARY compatSelfTests=pass modes=default,legacy-fallback exitCode=0"
}

exit 0
