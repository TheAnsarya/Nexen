param(
	[int]$Runs = 3,
	[string[]]$RuleIds = @("MD022", "MD031", "MD032", "MD047")
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command markdownlint -ErrorAction SilentlyContinue)) {
	Write-Error "markdownlint was not found on PATH. Install markdownlint-cli before running this benchmark."
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

$skipPattern = "[\\/](\.git|node_modules|bin|obj|BenchmarkDotNet\.Artifacts|\.venv|vcpkg_installed)[\\/]"
$fileCount = @(Get-ChildItem -Path . -Recurse -File -Filter "*.md" | Where-Object { $_.FullName -notmatch $skipPattern }).Count
$pattern = [string]::Join("|", $RuleIds)
$timesMs = @()

for ($i = 1; $i -le $Runs; $i++) {
	$sw = [System.Diagnostics.Stopwatch]::StartNew()
	$output = & markdownlint @args 2>&1
	$sw.Stop()
	$matches = @($output | Select-String -Pattern $pattern)
	$timesMs += $sw.Elapsed.TotalMilliseconds
	Write-Host ("Run {0}: {1:N2} ms, targeted violations={2}" -f $i, $sw.Elapsed.TotalMilliseconds, $matches.Count)
}

$average = ($timesMs | Measure-Object -Average).Average
$min = ($timesMs | Measure-Object -Minimum).Minimum
$max = ($timesMs | Measure-Object -Maximum).Maximum
$filesPerSecond = if ($average -gt 0) { $fileCount / ($average / 1000.0) } else { 0 }

Write-Host ""
Write-Host "Markdown policy benchmark summary"
Write-Host ("Files scanned: {0}" -f $fileCount)
Write-Host ("Rules: {0}" -f ($RuleIds -join ", "))
Write-Host ("Runs: {0}" -f $Runs)
Write-Host ("Average: {0:N2} ms" -f $average)
Write-Host ("Min: {0:N2} ms" -f $min)
Write-Host ("Max: {0:N2} ms" -f $max)
Write-Host ("Throughput: {0:N2} files/sec" -f $filesPerSecond)

exit 0
