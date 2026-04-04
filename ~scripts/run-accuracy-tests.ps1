#!/usr/bin/env pwsh
# Nexen Accuracy Test Runner
# Reads test manifests and executes each test ROM through Nexen headless mode.
# Produces a JSON results file and console summary.
#
# Usage:
#   .\run-accuracy-tests.ps1 -All
#   .\run-accuracy-tests.ps1 -System nes
#   .\run-accuracy-tests.ps1 -System gb -Tag critical
#   .\run-accuracy-tests.ps1 -System nes -Tag blargg -Verbose

[CmdletBinding()]
param(
	[switch]$All,
	[string]$System,
	[string]$Tag,
	[string]$NexenPath,
	[string]$RomRoot = "C:\~reference-roms",
	[string]$ResultsPath
)

$ErrorActionPreference = "Stop"
$ScriptRoot = $PSScriptRoot

# Resolve Nexen executable
if (-not $NexenPath) {
	$candidates = @(
		(Join-Path $ScriptRoot "..\bin\win-x64\Release\Nexen.exe"),
		(Join-Path $ScriptRoot "..\bin\win-x64\Debug\Nexen.exe")
	)
	foreach ($c in $candidates) {
		if (Test-Path $c) {
			$NexenPath = (Resolve-Path $c).Path
			break
		}
	}
	if (-not $NexenPath) {
		Write-Error "Could not find Nexen.exe. Specify -NexenPath."
		exit 1
	}
}

if (-not (Test-Path $NexenPath)) {
	Write-Error "Nexen not found at: $NexenPath"
	exit 1
}

Write-Host "Nexen: $NexenPath" -ForegroundColor Cyan
Write-Host "ROM root: $RomRoot" -ForegroundColor Cyan

# Find all manifest files
$manifestFiles = Get-ChildItem -Path $ScriptRoot -Filter "*-accuracy-tests.json" -File
if ($manifestFiles.Count -eq 0) {
	Write-Error "No manifest files found in $ScriptRoot"
	exit 1
}

Write-Host "Found $($manifestFiles.Count) manifest(s): $($manifestFiles.Name -join ', ')" -ForegroundColor Cyan

# Load and merge all tests
$allTests = @()
foreach ($mf in $manifestFiles) {
	$manifest = Get-Content $mf.FullName -Raw | ConvertFrom-Json
	foreach ($test in $manifest.tests) {
		$allTests += $test
	}
}

Write-Host "Total tests loaded: $($allTests.Count)" -ForegroundColor Cyan

# Filter by system
if ($System) {
	$allTests = $allTests | Where-Object { $_.system -eq $System }
	Write-Host "Filtered to system '$System': $($allTests.Count) tests" -ForegroundColor Yellow
}

# Filter by tag
if ($Tag) {
	$allTests = $allTests | Where-Object { $_.tags -contains $Tag }
	Write-Host "Filtered to tag '$Tag': $($allTests.Count) tests" -ForegroundColor Yellow
}

if (-not $All -and -not $System -and -not $Tag) {
	Write-Error "Specify -All, -System, or -Tag to select tests."
	exit 1
}

# Filter disabled tests
$allTests = $allTests | Where-Object { $_.PSObject.Properties['enabled'] -eq $null -or $_.enabled -eq $true }

if ($allTests.Count -eq 0) {
	Write-Host "No tests match the specified filters." -ForegroundColor Yellow
	exit 0
}

# Results collection
$results = @()
$passCount = 0
$failCount = 0
$skipCount = 0
$timeoutCount = 0
$startTime = Get-Date

Write-Host ""
Write-Host "Running $($allTests.Count) tests..." -ForegroundColor White
Write-Host ("=" * 72) -ForegroundColor DarkGray

foreach ($test in $allTests) {
	$romPath = Join-Path $RomRoot $test.rom

	# Check ROM exists
	if (-not (Test-Path $romPath)) {
		Write-Host "  SKIP  $($test.name) (ROM not found)" -ForegroundColor DarkYellow
		$results += [PSCustomObject]@{
			name = $test.name
			system = $test.system
			result = "skip"
			exitCode = -1
			duration = 0
			message = "ROM not found: $($test.rom)"
		}
		$skipCount++
		continue
	}

	$testStart = Get-Date
	$exitCode = -1
	$resultStatus = "error"
	$message = ""

	try {
		switch ($test.strategy) {
			"memory-read" {
				# Use RunTest via headless mode with address/memType
				$addr = [Convert]::ToInt32($test.address, 16)
				$frameCount = if ($test.maxFrames) { $test.maxFrames } else { 500 }
				$earlyExitByte = if ($test.PSObject.Properties['runningValue']) {
					[Convert]::ToInt32($test.runningValue, 16)
				} else { -1 }

				$passVal = [Convert]::ToInt32($test.passValue, 16)

				# Run via Lua script with configured globals
				$luaScript = Join-Path $ScriptRoot "common\memory-poll-test.lua"
				$luaSetup = "TEST_ADDRESS=$addr; TEST_MEM_TYPE=emu.memType.$($test.memType); TEST_PASS_VALUE=$passVal"
				if ($earlyExitByte -ge 0) {
					$luaSetup += "; TEST_RUN_VALUE=$earlyExitByte"
				}
				$luaSetup += "; TEST_MAX_FRAMES=$frameCount"

				$proc = Start-Process -FilePath $NexenPath -ArgumentList @(
					"--novideo", "--noaudio", "--noinput",
					"--lua", $luaScript,
					"--lua-globals", $luaSetup,
					$romPath
				) -Wait -PassThru -NoNewWindow
				$exitCode = $proc.ExitCode
			}
			"lua-script" {
				$luaScript = Join-Path $ScriptRoot $test.script
				if (-not (Test-Path $luaScript)) {
					$message = "Lua script not found: $($test.script)"
					$resultStatus = "skip"
					$skipCount++
					$results += [PSCustomObject]@{
						name = $test.name
						system = $test.system
						result = $resultStatus
						exitCode = -1
						duration = 0
						message = $message
					}
					Write-Host "  SKIP  $($test.name) ($message)" -ForegroundColor DarkYellow
					continue
				}

				$proc = Start-Process -FilePath $NexenPath -ArgumentList @(
					"--novideo", "--noaudio", "--noinput",
					"--lua", $luaScript,
					$romPath
				) -Wait -PassThru -NoNewWindow
				$exitCode = $proc.ExitCode
			}
			default {
				$message = "Unsupported strategy: $($test.strategy)"
				$resultStatus = "skip"
				$skipCount++
				$results += [PSCustomObject]@{
					name = $test.name
					system = $test.system
					result = $resultStatus
					exitCode = -1
					duration = 0
					message = $message
				}
				Write-Host "  SKIP  $($test.name) ($message)" -ForegroundColor DarkYellow
				continue
			}
		}

		$testDuration = ((Get-Date) - $testStart).TotalSeconds

		# Map exit codes
		switch ($exitCode) {
			0 {
				$resultStatus = "pass"
				$message = "PASS"
				$passCount++
				Write-Host "  PASS  $($test.name) ($([math]::Round($testDuration, 1))s)" -ForegroundColor Green
			}
			1 {
				$resultStatus = "fail"
				$message = "FAIL"
				$failCount++
				Write-Host "  FAIL  $($test.name) ($([math]::Round($testDuration, 1))s)" -ForegroundColor Red
			}
			2 {
				$resultStatus = "timeout"
				$message = "TIMEOUT"
				$timeoutCount++
				Write-Host "  TIME  $($test.name) ($([math]::Round($testDuration, 1))s)" -ForegroundColor DarkYellow
			}
			90 {
				$resultStatus = "error"
				$message = "Wrong console type"
				$failCount++
				Write-Host "  ERR   $($test.name) (wrong console type)" -ForegroundColor Red
			}
			default {
				$resultStatus = "error"
				$message = "Exit code: $exitCode"
				$failCount++
				Write-Host "  ERR   $($test.name) (exit $exitCode)" -ForegroundColor Red
			}
		}
	} catch {
		$testDuration = ((Get-Date) - $testStart).TotalSeconds
		$resultStatus = "error"
		$message = $_.Exception.Message
		$failCount++
		Write-Host "  ERR   $($test.name) ($message)" -ForegroundColor Red
	}

	$results += [PSCustomObject]@{
		name = $test.name
		system = $test.system
		result = $resultStatus
		exitCode = $exitCode
		duration = [math]::Round(((Get-Date) - $testStart).TotalSeconds, 2)
		message = $message
	}
}

$totalDuration = ((Get-Date) - $startTime).TotalSeconds

# Print summary
Write-Host ("=" * 72) -ForegroundColor DarkGray
Write-Host ""
Write-Host "Results: $passCount passed, $failCount failed, $timeoutCount timeout, $skipCount skipped" -ForegroundColor White
Write-Host "Total: $($results.Count) tests in $([math]::Round($totalDuration, 1))s" -ForegroundColor White

if ($failCount -gt 0) {
	Write-Host ""
	Write-Host "Failed tests:" -ForegroundColor Red
	$results | Where-Object { $_.result -eq "fail" -or $_.result -eq "error" } | ForEach-Object {
		Write-Host "  - $($_.name): $($_.message)" -ForegroundColor Red
	}
}

# Write results JSON
if (-not $ResultsPath) {
	$ResultsPath = Join-Path $ScriptRoot "accuracy-results.json"
}

$output = [PSCustomObject]@{
	timestamp = (Get-Date -Format "o")
	nexenPath = $NexenPath
	romRoot = $RomRoot
	filters = [PSCustomObject]@{
		system = $System
		tag = $Tag
		all = $All.IsPresent
	}
	summary = [PSCustomObject]@{
		total = $results.Count
		passed = $passCount
		failed = $failCount
		timeout = $timeoutCount
		skipped = $skipCount
		durationSeconds = [math]::Round($totalDuration, 2)
	}
	results = $results
}

$output | ConvertTo-Json -Depth 5 | Set-Content -Path $ResultsPath -Encoding UTF8
Write-Host ""
Write-Host "Results written to: $ResultsPath" -ForegroundColor Cyan

# Exit with failure if any tests failed
if ($failCount -gt 0) {
	exit 1
}
exit 0
