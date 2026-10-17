param(
	[string[]]$RuleIds = @("MD022", "MD031", "MD032", "MD047")
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command markdownlint -ErrorAction SilentlyContinue)) {
	Write-Error "markdownlint was not found on PATH. Install markdownlint-cli before running this test."
	exit 2
}

$args = @(
	"**/*.md",
	"--ignore", "**/.git/**",
	"--ignore", "**/node_modules/**",
	"--ignore", "**/bin/**",
	"--ignore", "**/obj/**",
	"--ignore", "**/BenchmarkDotNet.Artifacts/**",
	"--ignore", "**/.venv/**",
	"--ignore", "**/vcpkg_installed/**"
)

$output = & markdownlint @args 2>&1
$pattern = [string]::Join("|", $RuleIds)
$matches = @($output | Select-String -Pattern $pattern)

if ($matches.Count -gt 0) {
	Write-Host "Markdown policy test failed for rules: $($RuleIds -join ', ')"
	$matches | ForEach-Object {
		Write-Host $_.Line
	}
	exit 1
}

Write-Host "Markdown policy test passed for rules: $($RuleIds -join ', ')"
exit 0
