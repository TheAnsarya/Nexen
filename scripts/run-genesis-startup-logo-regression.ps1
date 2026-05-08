param(
	[string[]]$RomPaths = @(
		"C:\~reference-roms\genesis\md\Sonic The Hedgehog (W) (REV00) [!].md",
		"C:\~reference-roms\genesis\md\Sonic The Hedgehog (W) (REV01) [!].md",
		"C:\~reference-roms\genesis\md\Sonic The Hedgehog 2 (W) (REV01) [!].md"
	),
	[string]$CompareScriptPath = ".\scripts\run-genesis-cross-emu-wram-compare.ps1",
	[string]$OutputDirectory = ".\reference\startup-logo-regression",
	[string]$BaselinePath = ".\reference\startup-logo-baseline.json",
	[int]$FrameStart = 0,
	[int]$FrameEnd = 600,
	[int]$AutoStopTimeoutSeconds = 45,
	[int]$MaxLines = 1200000,
	[string[]]$StartupProfiles = @("logo-compat"),
	[switch]$UpdateBaseline,
	[switch]$AllowMissingMesenFrontend,
	[switch]$DisableMesenFallbackRunModes,
	[switch]$StrictRequireMesenTraces,
	[switch]$StrictStartupParity,
	[switch]$FailOnWramDiff,
	[switch]$FailOnStartupDiff,
	[switch]$FailOnMissingStartupMetrics,
	[int]$MinNexenStartupCheckpointCount = 1,
	[int]$MinNexenStartupDisplayTransitionCount = 0,
	[int]$MinNexenStartupPaletteCheckpointCount = 1,
	[int]$MinNexenStartupVdpSnapshotCount = 1,
	[int]$MinNexenStartupVdpRegisterWriteCount = 1,
	[int]$MinNexenStartupTmssUnlockCount = 0,
	[int]$MinNexenStartupZ80RuntimeToggleCount = 0,
	[switch]$StrictRequireMesenStartupEvents,
	[switch]$StrictRequireBothStartupCheckpointEvents,
	[switch]$StrictRequireBothStartupDisplayTransitionEvents,
	[switch]$StrictRequireBothStartupPaletteCheckpointEvents,
	[switch]$StrictRequireBothStartupVdpSnapshotEvents,
	[switch]$StrictRequireBothStartupVdpRegisterWriteEvents,
	[switch]$StrictRequireBothStartupTmssUnlockEvents,
	[switch]$StrictRequireBothStartupZ80RuntimeToggleEvents,
	[int]$MinMesenStartupCheckpointCount = 1,
	[int]$MinMesenStartupDisplayTransitionCount = 0,
	[int]$MinMesenStartupPaletteCheckpointCount = 1,
	[int]$MinMesenStartupVdpSnapshotCount = 1,
	[int]$MinMesenStartupVdpRegisterWriteCount = 1,
	[int]$MinMesenStartupTmssUnlockCount = 0,
	[int]$MinMesenStartupZ80RuntimeToggleCount = 0
)

$ErrorActionPreference = "Stop"

function Resolve-PathRequired {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,
		[Parameter(Mandatory = $true)]
		[string]$Label
	)

	$resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
	if ($null -eq $resolved) {
		throw "Unable to resolve $Label path: $Path"
	}

	return $resolved.Path
}

function Resolve-PathOptional {
	param([string]$Path)

	if ([string]::IsNullOrWhiteSpace($Path)) {
		return $null
	}

	try {
		$resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
		if ($null -eq $resolved) {
			return $null
		}
		return $resolved.Path
	} catch {
		return $null
	}
}

function Convert-ReportToMap {
	param(
		[Parameter(Mandatory = $true)]
		[string]$ReportPath
	)

	$map = @{}
	foreach ($line in (Get-Content -LiteralPath $ReportPath -ErrorAction Stop)) {
		if ([string]::IsNullOrWhiteSpace($line)) {
			continue
		}

		$eq = $line.IndexOf('=')
		if ($eq -lt 1) {
			continue
		}

		$key = $line.Substring(0, $eq)
		$value = $line.Substring($eq + 1)
		$map[$key] = $value
	}

	return $map
}

function Get-BoolValue {
	param(
		$Value,
		[bool]$Default = $false
	)

	if ($null -eq $Value) {
		return $Default
	}

	$text = "$Value".Trim().ToLowerInvariant()
	if ($text -in @("1", "true", "yes", "on")) {
		return $true
	}
	if ($text -in @("0", "false", "no", "off")) {
		return $false
	}

	return $Default
}

function Get-IntValue {
	param(
		$Value,
		[int]$Default = 0
	)

	if ($null -eq $Value) {
		return $Default
	}

	$parsed = 0
	if ([int]::TryParse("$Value", [ref]$parsed)) {
		return $parsed
	}

	return $Default
}

function New-RomKey {
	param([string]$Path)

	if ([string]::IsNullOrWhiteSpace($Path)) {
		return "unknown"
	}

	$file = [System.IO.Path]::GetFileNameWithoutExtension($Path)
	if ([string]::IsNullOrWhiteSpace($file)) {
		$file = $Path
	}

	$normalized = [Regex]::Replace($file.ToLowerInvariant(), "[^a-z0-9]+", "-")
	return $normalized.Trim('-')
}

$resolvedCompareScriptPath = Resolve-PathRequired -Path $CompareScriptPath -Label "compare script"
$resolvedOutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
if (!(Test-Path -LiteralPath $resolvedOutputDirectory)) {
	New-Item -Path $resolvedOutputDirectory -ItemType Directory -Force | Out-Null
}

$resolvedBaselinePath = [System.IO.Path]::GetFullPath($BaselinePath)
$timestamp = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss")
$summaryJsonPath = Join-Path $resolvedOutputDirectory "startup-logo-regression-summary-$timestamp.json"
$summaryMarkdownPath = Join-Path $resolvedOutputDirectory "startup-logo-regression-summary-$timestamp.md"
$latestJsonPath = Join-Path $resolvedOutputDirectory "startup-logo-regression-summary.latest.json"
$latestMarkdownPath = Join-Path $resolvedOutputDirectory "startup-logo-regression-summary.latest.md"

$baseline = @{}
if ((Test-Path -LiteralPath $resolvedBaselinePath) -and -not $UpdateBaseline) {
	$baselineRaw = Get-Content -LiteralPath $resolvedBaselinePath -Raw -ErrorAction Stop
	if (-not [string]::IsNullOrWhiteSpace($baselineRaw)) {
		$baseline = $baselineRaw | ConvertFrom-Json -AsHashtable
	}
}

$results = New-Object System.Collections.Generic.List[object]
$regressionCount = 0
$missingMesenCount = 0
$policyFailureCount = 0
$anyErrors = $false

if ($null -eq $StartupProfiles -or $StartupProfiles.Count -eq 0) {
	$StartupProfiles = @("logo-compat")
}

if ($StrictStartupParity) {
	$StrictRequireMesenTraces = $true
	$StrictRequireBothStartupCheckpointEvents = $true
	$StrictRequireBothStartupDisplayTransitionEvents = $true
	$StrictRequireBothStartupPaletteCheckpointEvents = $true
	$StrictRequireBothStartupVdpSnapshotEvents = $true
	$StrictRequireBothStartupVdpRegisterWriteEvents = $true
	$StrictRequireBothStartupTmssUnlockEvents = $true
	$StrictRequireBothStartupZ80RuntimeToggleEvents = $true

	$MinNexenStartupCheckpointCount = [Math]::Max($MinNexenStartupCheckpointCount, 1)
	$MinMesenStartupCheckpointCount = [Math]::Max($MinMesenStartupCheckpointCount, 1)
	$MinNexenStartupDisplayTransitionCount = [Math]::Max($MinNexenStartupDisplayTransitionCount, 1)
	$MinMesenStartupDisplayTransitionCount = [Math]::Max($MinMesenStartupDisplayTransitionCount, 1)
	$MinNexenStartupPaletteCheckpointCount = [Math]::Max($MinNexenStartupPaletteCheckpointCount, 1)
	$MinMesenStartupPaletteCheckpointCount = [Math]::Max($MinMesenStartupPaletteCheckpointCount, 1)
	$MinNexenStartupVdpSnapshotCount = [Math]::Max($MinNexenStartupVdpSnapshotCount, 1)
	$MinMesenStartupVdpSnapshotCount = [Math]::Max($MinMesenStartupVdpSnapshotCount, 1)
	$MinNexenStartupVdpRegisterWriteCount = [Math]::Max($MinNexenStartupVdpRegisterWriteCount, 1)
	$MinMesenStartupVdpRegisterWriteCount = [Math]::Max($MinMesenStartupVdpRegisterWriteCount, 1)
	$MinNexenStartupZ80RuntimeToggleCount = [Math]::Max($MinNexenStartupZ80RuntimeToggleCount, 1)
	$MinMesenStartupZ80RuntimeToggleCount = [Math]::Max($MinMesenStartupZ80RuntimeToggleCount, 1)
}

foreach ($rom in $RomPaths) {
	if ([string]::IsNullOrWhiteSpace($rom)) {
		continue
	}

	$resolvedRom = Resolve-PathOptional -Path $rom
	if ([string]::IsNullOrWhiteSpace($resolvedRom)) {
		Write-Warning "Skipping missing ROM path: $rom"
		continue
	}
	$romKey = New-RomKey -Path $resolvedRom
	foreach ($profile in $StartupProfiles) {
		$profileName = if ([string]::IsNullOrWhiteSpace($profile)) { "logo-compat" } else { $profile }
		$profileKey = [Regex]::Replace($profileName.ToLowerInvariant(), "[^a-z0-9]+", "-").Trim('-')
		if ([string]::IsNullOrWhiteSpace($profileKey)) {
			$profileKey = "logo-compat"
		}

		$runReportPath = Join-Path $resolvedOutputDirectory "cross-emu-$romKey-$profileKey-$timestamp.txt"

		$runnerArgs = @(
			"-NoProfile",
			"-ExecutionPolicy", "Bypass",
			"-File", $resolvedCompareScriptPath,
			"-RomPath", $resolvedRom,
			"-FrameStart", "$FrameStart",
			"-FrameEnd", "$FrameEnd",
			"-AutoStopTimeoutSeconds", "$AutoStopTimeoutSeconds",
			"-MaxLines", "$MaxLines",
			"-ReportPath", $runReportPath
		)

		if ($AllowMissingMesenFrontend) {
			$runnerArgs += "-AllowMissingMesenFrontend"
		}
		if ($DisableMesenFallbackRunModes) {
			$runnerArgs += "-DisableMesenFallbackRunModes"
		}

		$oldProfile = $env:NEXEN_GENESIS_STARTUP_PROFILE
		try {
			$env:NEXEN_GENESIS_STARTUP_PROFILE = $profileName
			Write-Host "Running startup/logo compare for $resolvedRom (profile=$profileName)" -ForegroundColor Cyan
			& pwsh @runnerArgs
			$compareExitCode = $LASTEXITCODE
		} finally {
			$env:NEXEN_GENESIS_STARTUP_PROFILE = $oldProfile
		}

		$report = Convert-ReportToMap -ReportPath $runReportPath
		$mesenMissing = Get-BoolValue -Value $report["mesenTraceMissing"]
		$mesenStartupMissing = Get-BoolValue -Value $report["mesenStartupTraceMissing"]
		$mesenSkipped = Get-BoolValue -Value $report["mesenRunSkipped"]

		if ($mesenMissing -or $mesenStartupMissing -or $mesenSkipped) {
			$missingMesenCount++
		}

		$current = [ordered]@{
			romKey = $romKey
			profile = $profileName
			profileKey = $profileKey
			baselineKey = "$romKey::$profileKey"
			romPath = $resolvedRom
			reportPath = $runReportPath
			compareExitCode = $compareExitCode
			firstDiffIndex = Get-IntValue -Value $report["firstDiffIndex"] -Default -1
			startupFirstDiffIndex = Get-IntValue -Value $report["startupFirstDiffIndex"] -Default -1
			nexenHash = $report["nexenHash"]
			mesenHash = $report["mesenHash"]
			nexenStartupHash = $report["nexenStartupHash"]
			mesenStartupHash = $report["mesenStartupHash"]
			nexenLines = Get-IntValue -Value $report["nexenLines"]
			mesenLines = Get-IntValue -Value $report["mesenLines"]
			nexenStartupLines = Get-IntValue -Value $report["nexenStartupLines"]
			mesenStartupLines = Get-IntValue -Value $report["mesenStartupLines"]
			nexenStartupCheckpointCount = Get-IntValue -Value $report["nexenStartupCheckpointCount"]
			mesenStartupCheckpointCount = Get-IntValue -Value $report["mesenStartupCheckpointCount"]
			nexenStartupDisplayTransitionCount = Get-IntValue -Value $report["nexenStartupDisplayTransitionCount"]
			mesenStartupDisplayTransitionCount = Get-IntValue -Value $report["mesenStartupDisplayTransitionCount"]
			nexenStartupPaletteCheckpointCount = Get-IntValue -Value $report["nexenStartupPaletteCheckpointCount"]
			mesenStartupPaletteCheckpointCount = Get-IntValue -Value $report["mesenStartupPaletteCheckpointCount"]
			nexenStartupVdpSnapshotCount = Get-IntValue -Value $report["nexenStartupVdpSnapshotCount"]
			mesenStartupVdpSnapshotCount = Get-IntValue -Value $report["mesenStartupVdpSnapshotCount"]
			nexenStartupVdpRegisterWriteCount = Get-IntValue -Value $report["nexenStartupVdpRegisterWriteCount"]
			mesenStartupVdpRegisterWriteCount = Get-IntValue -Value $report["mesenStartupVdpRegisterWriteCount"]
			nexenStartupTmssUnlockCount = Get-IntValue -Value $report["nexenStartupTmssUnlockCount"]
			mesenStartupTmssUnlockCount = Get-IntValue -Value $report["mesenStartupTmssUnlockCount"]
			nexenStartupZ80RuntimeToggleCount = Get-IntValue -Value $report["nexenStartupZ80RuntimeToggleCount"]
			mesenStartupZ80RuntimeToggleCount = Get-IntValue -Value $report["mesenStartupZ80RuntimeToggleCount"]
			mesenTraceMissing = $mesenMissing
			mesenStartupTraceMissing = $mesenStartupMissing
			mesenRunSkipped = $mesenSkipped
		}

		$baselineEntry = $null
		if ($baseline.ContainsKey($current.baselineKey)) {
			$baselineEntry = $baseline[$current.baselineKey]
		}

		$regressions = New-Object System.Collections.Generic.List[string]
		if ($null -ne $baselineEntry) {
			if ($baselineEntry.nexenHash -and $baselineEntry.nexenHash -ne $current.nexenHash) {
				$regressions.Add("nexenHash")
			}
			if ($baselineEntry.nexenStartupHash -and $baselineEntry.nexenStartupHash -ne $current.nexenStartupHash) {
				$regressions.Add("nexenStartupHash")
			}
			if ($baselineEntry.nexenStartupCheckpointCount -ne $null -and [int]$baselineEntry.nexenStartupCheckpointCount -ne $current.nexenStartupCheckpointCount) {
				$regressions.Add("nexenStartupCheckpointCount")
			}
			if ($baselineEntry.nexenStartupDisplayTransitionCount -ne $null -and [int]$baselineEntry.nexenStartupDisplayTransitionCount -ne $current.nexenStartupDisplayTransitionCount) {
				$regressions.Add("nexenStartupDisplayTransitionCount")
			}
			if ($baselineEntry.nexenStartupPaletteCheckpointCount -ne $null -and [int]$baselineEntry.nexenStartupPaletteCheckpointCount -ne $current.nexenStartupPaletteCheckpointCount) {
				$regressions.Add("nexenStartupPaletteCheckpointCount")
			}
			if ($baselineEntry.nexenStartupVdpSnapshotCount -ne $null -and [int]$baselineEntry.nexenStartupVdpSnapshotCount -ne $current.nexenStartupVdpSnapshotCount) {
				$regressions.Add("nexenStartupVdpSnapshotCount")
			}
			if ($baselineEntry.nexenStartupVdpRegisterWriteCount -ne $null -and [int]$baselineEntry.nexenStartupVdpRegisterWriteCount -ne $current.nexenStartupVdpRegisterWriteCount) {
				$regressions.Add("nexenStartupVdpRegisterWriteCount")
			}
			if ($baselineEntry.nexenStartupTmssUnlockCount -ne $null -and [int]$baselineEntry.nexenStartupTmssUnlockCount -ne $current.nexenStartupTmssUnlockCount) {
				$regressions.Add("nexenStartupTmssUnlockCount")
			}
			if ($baselineEntry.nexenStartupZ80RuntimeToggleCount -ne $null -and [int]$baselineEntry.nexenStartupZ80RuntimeToggleCount -ne $current.nexenStartupZ80RuntimeToggleCount) {
				$regressions.Add("nexenStartupZ80RuntimeToggleCount")
			}
		}

		$current["regressions"] = @($regressions)
		$current["regressionCount"] = $regressions.Count
		if ($regressions.Count -gt 0) {
			$regressionCount++
		}

		if ($compareExitCode -ne 0 -and $compareExitCode -ne 3) {
			$anyErrors = $true
		}

		$policyFailures = New-Object System.Collections.Generic.List[string]
		if ($StrictRequireMesenTraces -and ($mesenMissing -or $mesenStartupMissing -or $mesenSkipped)) {
			$policyFailures.Add("missingMesenTrace")
		}
		if ($FailOnWramDiff -and $current.firstDiffIndex -ge 0) {
			$policyFailures.Add("wramDiff")
		}
		if ($FailOnStartupDiff -and $current.startupFirstDiffIndex -ge 0) {
			$policyFailures.Add("startupDiff")
		}
		if ($FailOnMissingStartupMetrics) {
			if ($current.nexenStartupCheckpointCount -lt $MinNexenStartupCheckpointCount) {
				$policyFailures.Add("nexenStartupCheckpointCount")
			}
			if ($current.nexenStartupDisplayTransitionCount -lt $MinNexenStartupDisplayTransitionCount) {
				$policyFailures.Add("nexenStartupDisplayTransitionCount")
			}
			if ($current.nexenStartupPaletteCheckpointCount -lt $MinNexenStartupPaletteCheckpointCount) {
				$policyFailures.Add("nexenStartupPaletteCheckpointCount")
			}
			if ($current.nexenStartupVdpSnapshotCount -lt $MinNexenStartupVdpSnapshotCount) {
				$policyFailures.Add("nexenStartupVdpSnapshotCount")
			}
			if ($current.nexenStartupVdpRegisterWriteCount -lt $MinNexenStartupVdpRegisterWriteCount) {
				$policyFailures.Add("nexenStartupVdpRegisterWriteCount")
			}
			if ($current.nexenStartupTmssUnlockCount -lt $MinNexenStartupTmssUnlockCount) {
				$policyFailures.Add("nexenStartupTmssUnlockCount")
			}
			if ($current.nexenStartupZ80RuntimeToggleCount -lt $MinNexenStartupZ80RuntimeToggleCount) {
				$policyFailures.Add("nexenStartupZ80RuntimeToggleCount")
			}
		}
		if ($StrictRequireMesenStartupEvents) {
			if ($current.mesenStartupCheckpointCount -lt $MinMesenStartupCheckpointCount) {
				$policyFailures.Add("mesenStartupCheckpointCount")
			}
			if ($current.mesenStartupDisplayTransitionCount -lt $MinMesenStartupDisplayTransitionCount) {
				$policyFailures.Add("mesenStartupDisplayTransitionCount")
			}
		}
		if ($StrictRequireBothStartupCheckpointEvents) {
			if ($current.mesenStartupCheckpointCount -lt $MinMesenStartupCheckpointCount) {
				$policyFailures.Add("dualStartupCheckpoint.mesen")
			}
			if ($current.nexenStartupCheckpointCount -lt $MinNexenStartupCheckpointCount) {
				$policyFailures.Add("dualStartupCheckpoint.nexen")
			}
		}
		if ($StrictRequireBothStartupDisplayTransitionEvents) {
			if ($current.mesenStartupDisplayTransitionCount -lt $MinMesenStartupDisplayTransitionCount) {
				$policyFailures.Add("dualStartupDisplayTransition.mesen")
			}
			if ($current.nexenStartupDisplayTransitionCount -lt $MinNexenStartupDisplayTransitionCount) {
				$policyFailures.Add("dualStartupDisplayTransition.nexen")
			}
		}
		if ($StrictRequireBothStartupPaletteCheckpointEvents) {
			if ($current.mesenStartupPaletteCheckpointCount -lt $MinMesenStartupPaletteCheckpointCount) {
				$policyFailures.Add("dualStartupPaletteCheckpoint.mesen")
			}
			if ($current.nexenStartupPaletteCheckpointCount -lt $MinNexenStartupPaletteCheckpointCount) {
				$policyFailures.Add("dualStartupPaletteCheckpoint.nexen")
			}
		}
		if ($StrictRequireBothStartupVdpSnapshotEvents) {
			if ($current.mesenStartupVdpSnapshotCount -lt $MinMesenStartupVdpSnapshotCount) {
				$policyFailures.Add("dualStartupVdpSnapshot.mesen")
			}
			if ($current.nexenStartupVdpSnapshotCount -lt $MinNexenStartupVdpSnapshotCount) {
				$policyFailures.Add("dualStartupVdpSnapshot.nexen")
			}
		}
		if ($StrictRequireBothStartupVdpRegisterWriteEvents) {
			if ($current.mesenStartupVdpRegisterWriteCount -lt $MinMesenStartupVdpRegisterWriteCount) {
				$policyFailures.Add("dualStartupVdpRegisterWrite.mesen")
			}
			if ($current.nexenStartupVdpRegisterWriteCount -lt $MinNexenStartupVdpRegisterWriteCount) {
				$policyFailures.Add("dualStartupVdpRegisterWrite.nexen")
			}
		}
		if ($StrictRequireBothStartupTmssUnlockEvents) {
			if ($current.mesenStartupTmssUnlockCount -lt $MinMesenStartupTmssUnlockCount) {
				$policyFailures.Add("dualStartupTmssUnlock.mesen")
			}
			if ($current.nexenStartupTmssUnlockCount -lt $MinNexenStartupTmssUnlockCount) {
				$policyFailures.Add("dualStartupTmssUnlock.nexen")
			}
		}
		if ($StrictRequireBothStartupZ80RuntimeToggleEvents) {
			if ($current.mesenStartupZ80RuntimeToggleCount -lt $MinMesenStartupZ80RuntimeToggleCount) {
				$policyFailures.Add("dualStartupZ80RuntimeToggle.mesen")
			}
			if ($current.nexenStartupZ80RuntimeToggleCount -lt $MinNexenStartupZ80RuntimeToggleCount) {
				$policyFailures.Add("dualStartupZ80RuntimeToggle.nexen")
			}
		}

		$current["policyFailures"] = @($policyFailures)
		$current["policyFailureCount"] = $policyFailures.Count
		$current["verdict"] = if ($policyFailures.Count -gt 0) { "policy-fail" } else { "pass" }
		if ($policyFailures.Count -gt 0) {
			$policyFailureCount++
		}

		$results.Add([pscustomobject]$current)
	}
}

$baselineOut = [ordered]@{}
foreach ($item in $results) {
	$baselineOut[$item.baselineKey] = [ordered]@{
		nexenHash = $item.nexenHash
		nexenStartupHash = $item.nexenStartupHash
		nexenStartupCheckpointCount = $item.nexenStartupCheckpointCount
		nexenStartupDisplayTransitionCount = $item.nexenStartupDisplayTransitionCount
		nexenStartupPaletteCheckpointCount = $item.nexenStartupPaletteCheckpointCount
		nexenStartupVdpSnapshotCount = $item.nexenStartupVdpSnapshotCount
		nexenStartupVdpRegisterWriteCount = $item.nexenStartupVdpRegisterWriteCount
		nexenStartupTmssUnlockCount = $item.nexenStartupTmssUnlockCount
		nexenStartupZ80RuntimeToggleCount = $item.nexenStartupZ80RuntimeToggleCount
	}
}

if ($UpdateBaseline) {
	$baselineJson = $baselineOut | ConvertTo-Json -Depth 6
	$baselineJson | Out-File -LiteralPath $resolvedBaselinePath -Encoding utf8
}

if ($results.Count -eq 0) {
	throw "No ROMs were resolved. Provide at least one valid -RomPaths entry."
}

$summary = New-Object PSObject
$resultArray = New-Object System.Collections.Generic.List[object]
foreach ($entry in $results) {
	$resultArray.Add($entry)
}
$summary | Add-Member -NotePropertyName "generatedUtc" -NotePropertyValue ([DateTime]::UtcNow.ToString("o"))
$summary | Add-Member -NotePropertyName "frameStart" -NotePropertyValue $FrameStart
$summary | Add-Member -NotePropertyName "frameEnd" -NotePropertyValue $FrameEnd
$summary | Add-Member -NotePropertyName "autoStopTimeoutSeconds" -NotePropertyValue $AutoStopTimeoutSeconds
$summary | Add-Member -NotePropertyName "maxLines" -NotePropertyValue $MaxLines
$summary | Add-Member -NotePropertyName "romCount" -NotePropertyValue $results.Count
$summary | Add-Member -NotePropertyName "regressionCount" -NotePropertyValue $regressionCount
$summary | Add-Member -NotePropertyName "missingMesenCount" -NotePropertyValue $missingMesenCount
$summary | Add-Member -NotePropertyName "policyFailureCount" -NotePropertyValue $policyFailureCount
$summary | Add-Member -NotePropertyName "anyErrors" -NotePropertyValue $anyErrors
$summary | Add-Member -NotePropertyName "startupProfiles" -NotePropertyValue @($StartupProfiles)
$summary | Add-Member -NotePropertyName "strictRequireMesenTraces" -NotePropertyValue ([bool]$StrictRequireMesenTraces)
$summary | Add-Member -NotePropertyName "strictStartupParity" -NotePropertyValue ([bool]$StrictStartupParity)
$summary | Add-Member -NotePropertyName "failOnWramDiff" -NotePropertyValue ([bool]$FailOnWramDiff)
$summary | Add-Member -NotePropertyName "failOnStartupDiff" -NotePropertyValue ([bool]$FailOnStartupDiff)
$summary | Add-Member -NotePropertyName "failOnMissingStartupMetrics" -NotePropertyValue ([bool]$FailOnMissingStartupMetrics)
$summary | Add-Member -NotePropertyName "strictRequireMesenStartupEvents" -NotePropertyValue ([bool]$StrictRequireMesenStartupEvents)
$summary | Add-Member -NotePropertyName "strictRequireBothStartupCheckpointEvents" -NotePropertyValue ([bool]$StrictRequireBothStartupCheckpointEvents)
$summary | Add-Member -NotePropertyName "strictRequireBothStartupDisplayTransitionEvents" -NotePropertyValue ([bool]$StrictRequireBothStartupDisplayTransitionEvents)
$summary | Add-Member -NotePropertyName "strictRequireBothStartupPaletteCheckpointEvents" -NotePropertyValue ([bool]$StrictRequireBothStartupPaletteCheckpointEvents)
$summary | Add-Member -NotePropertyName "strictRequireBothStartupVdpSnapshotEvents" -NotePropertyValue ([bool]$StrictRequireBothStartupVdpSnapshotEvents)
$summary | Add-Member -NotePropertyName "strictRequireBothStartupVdpRegisterWriteEvents" -NotePropertyValue ([bool]$StrictRequireBothStartupVdpRegisterWriteEvents)
$summary | Add-Member -NotePropertyName "strictRequireBothStartupTmssUnlockEvents" -NotePropertyValue ([bool]$StrictRequireBothStartupTmssUnlockEvents)
$summary | Add-Member -NotePropertyName "strictRequireBothStartupZ80RuntimeToggleEvents" -NotePropertyValue ([bool]$StrictRequireBothStartupZ80RuntimeToggleEvents)
$summary | Add-Member -NotePropertyName "minNexenStartupCheckpointCount" -NotePropertyValue $MinNexenStartupCheckpointCount
$summary | Add-Member -NotePropertyName "minNexenStartupDisplayTransitionCount" -NotePropertyValue $MinNexenStartupDisplayTransitionCount
$summary | Add-Member -NotePropertyName "minNexenStartupPaletteCheckpointCount" -NotePropertyValue $MinNexenStartupPaletteCheckpointCount
$summary | Add-Member -NotePropertyName "minNexenStartupVdpSnapshotCount" -NotePropertyValue $MinNexenStartupVdpSnapshotCount
$summary | Add-Member -NotePropertyName "minNexenStartupVdpRegisterWriteCount" -NotePropertyValue $MinNexenStartupVdpRegisterWriteCount
$summary | Add-Member -NotePropertyName "minNexenStartupTmssUnlockCount" -NotePropertyValue $MinNexenStartupTmssUnlockCount
$summary | Add-Member -NotePropertyName "minNexenStartupZ80RuntimeToggleCount" -NotePropertyValue $MinNexenStartupZ80RuntimeToggleCount
$summary | Add-Member -NotePropertyName "minMesenStartupCheckpointCount" -NotePropertyValue $MinMesenStartupCheckpointCount
$summary | Add-Member -NotePropertyName "minMesenStartupDisplayTransitionCount" -NotePropertyValue $MinMesenStartupDisplayTransitionCount
$summary | Add-Member -NotePropertyName "minMesenStartupPaletteCheckpointCount" -NotePropertyValue $MinMesenStartupPaletteCheckpointCount
$summary | Add-Member -NotePropertyName "minMesenStartupVdpSnapshotCount" -NotePropertyValue $MinMesenStartupVdpSnapshotCount
$summary | Add-Member -NotePropertyName "minMesenStartupVdpRegisterWriteCount" -NotePropertyValue $MinMesenStartupVdpRegisterWriteCount
$summary | Add-Member -NotePropertyName "minMesenStartupTmssUnlockCount" -NotePropertyValue $MinMesenStartupTmssUnlockCount
$summary | Add-Member -NotePropertyName "minMesenStartupZ80RuntimeToggleCount" -NotePropertyValue $MinMesenStartupZ80RuntimeToggleCount
$summary | Add-Member -NotePropertyName "baselinePath" -NotePropertyValue $resolvedBaselinePath
$summary | Add-Member -NotePropertyName "baselineUpdated" -NotePropertyValue ([bool]$UpdateBaseline)
$summary | Add-Member -NotePropertyName "results" -NotePropertyValue $resultArray.ToArray()

$summaryJson = $summary | ConvertTo-Json -Depth 8
$summaryJson | Out-File -LiteralPath $summaryJsonPath -Encoding utf8
$summaryJson | Out-File -LiteralPath $latestJsonPath -Encoding utf8

$md = New-Object System.Collections.Generic.List[string]
$md.Add("# Genesis Startup Logo Regression Summary")
$md.Add("")
$md.Add("- generatedUtc: $($summary.generatedUtc)")
$md.Add("- frameStart: $FrameStart")
$md.Add("- frameEnd: $FrameEnd")
$md.Add("- autoStopTimeoutSeconds: $AutoStopTimeoutSeconds")
$md.Add("- maxLines: $MaxLines")
$md.Add("- romCount: $($summary.romCount)")
$md.Add("- regressionCount: $regressionCount")
$md.Add("- missingMesenCount: $missingMesenCount")
$md.Add("- policyFailureCount: $policyFailureCount")
$md.Add("- anyErrors: $anyErrors")
$md.Add("- startupProfiles: $($StartupProfiles -join ',')")
$md.Add("- strictRequireMesenTraces: $([bool]$StrictRequireMesenTraces)")
$md.Add("- strictStartupParity: $([bool]$StrictStartupParity)")
$md.Add("- failOnWramDiff: $([bool]$FailOnWramDiff)")
$md.Add("- failOnStartupDiff: $([bool]$FailOnStartupDiff)")
$md.Add("- failOnMissingStartupMetrics: $([bool]$FailOnMissingStartupMetrics)")
$md.Add("- strictRequireMesenStartupEvents: $([bool]$StrictRequireMesenStartupEvents)")
$md.Add("- strictRequireBothStartupCheckpointEvents: $([bool]$StrictRequireBothStartupCheckpointEvents)")
$md.Add("- strictRequireBothStartupDisplayTransitionEvents: $([bool]$StrictRequireBothStartupDisplayTransitionEvents)")
$md.Add("- strictRequireBothStartupPaletteCheckpointEvents: $([bool]$StrictRequireBothStartupPaletteCheckpointEvents)")
$md.Add("- strictRequireBothStartupVdpSnapshotEvents: $([bool]$StrictRequireBothStartupVdpSnapshotEvents)")
$md.Add("- strictRequireBothStartupVdpRegisterWriteEvents: $([bool]$StrictRequireBothStartupVdpRegisterWriteEvents)")
$md.Add("- strictRequireBothStartupTmssUnlockEvents: $([bool]$StrictRequireBothStartupTmssUnlockEvents)")
$md.Add("- strictRequireBothStartupZ80RuntimeToggleEvents: $([bool]$StrictRequireBothStartupZ80RuntimeToggleEvents)")
$md.Add("- minNexenStartupCheckpointCount: $MinNexenStartupCheckpointCount")
$md.Add("- minNexenStartupDisplayTransitionCount: $MinNexenStartupDisplayTransitionCount")
$md.Add("- minNexenStartupPaletteCheckpointCount: $MinNexenStartupPaletteCheckpointCount")
$md.Add("- minNexenStartupVdpSnapshotCount: $MinNexenStartupVdpSnapshotCount")
$md.Add("- minNexenStartupVdpRegisterWriteCount: $MinNexenStartupVdpRegisterWriteCount")
$md.Add("- minNexenStartupTmssUnlockCount: $MinNexenStartupTmssUnlockCount")
$md.Add("- minNexenStartupZ80RuntimeToggleCount: $MinNexenStartupZ80RuntimeToggleCount")
$md.Add("- minMesenStartupCheckpointCount: $MinMesenStartupCheckpointCount")
$md.Add("- minMesenStartupDisplayTransitionCount: $MinMesenStartupDisplayTransitionCount")
$md.Add("- minMesenStartupPaletteCheckpointCount: $MinMesenStartupPaletteCheckpointCount")
$md.Add("- minMesenStartupVdpSnapshotCount: $MinMesenStartupVdpSnapshotCount")
$md.Add("- minMesenStartupVdpRegisterWriteCount: $MinMesenStartupVdpRegisterWriteCount")
$md.Add("- minMesenStartupTmssUnlockCount: $MinMesenStartupTmssUnlockCount")
$md.Add("- minMesenStartupZ80RuntimeToggleCount: $MinMesenStartupZ80RuntimeToggleCount")
$md.Add("")
$md.Add("| ROM | Profile | Exit | Diff | Startup Diff | Nexen Startup Hash | Mesen Startup Hash | N/M Chkpt | N/M Disp | N/M Pal | N/M VdpRegW | N/M Z80Tgl | N/M TmssUnlock | Regressions | Policy Failures | Verdict |")
$md.Add("|---|---|---:|---:|---:|---|---|---|---|---|---|---|---|---|---|---|")
foreach ($item in $results) {
	$regressionText = ""
	if ($item.regressionCount -gt 0) {
		$regressionText = ($item.regressions -join ",")
	}
	$policyText = ""
	if ($item.policyFailureCount -gt 0) {
		$policyText = ($item.policyFailures -join ",")
	}

	$md.Add("| $($item.romKey) | $($item.profileKey) | $($item.compareExitCode) | $($item.firstDiffIndex) | $($item.startupFirstDiffIndex) | $($item.nexenStartupHash) | $($item.mesenStartupHash) | $($item.nexenStartupCheckpointCount)/$($item.mesenStartupCheckpointCount) | $($item.nexenStartupDisplayTransitionCount)/$($item.mesenStartupDisplayTransitionCount) | $($item.nexenStartupPaletteCheckpointCount)/$($item.mesenStartupPaletteCheckpointCount) | $($item.nexenStartupVdpRegisterWriteCount)/$($item.mesenStartupVdpRegisterWriteCount) | $($item.nexenStartupZ80RuntimeToggleCount)/$($item.mesenStartupZ80RuntimeToggleCount) | $($item.nexenStartupTmssUnlockCount)/$($item.mesenStartupTmssUnlockCount) | $regressionText | $policyText | $($item.verdict) |")
}

$md.Add("")
$md.Add("## Notes")
$md.Add("")
$md.Add("- compareExitCode=3 indicates first-difference detection from the compare script.")
$md.Add("- missingMesenCount includes missing trace/startup-trace or skipped Mesen run.")
$md.Add("- policy failures are controlled by strict gate switches and do not imply infrastructure errors.")
$md.Add("- Use -UpdateBaseline to refresh baseline hashes and startup counters.")

$md | Out-File -LiteralPath $summaryMarkdownPath -Encoding utf8
$md | Out-File -LiteralPath $latestMarkdownPath -Encoding utf8

Write-Host "Startup/logo summary JSON: $summaryJsonPath" -ForegroundColor Green
Write-Host "Startup/logo summary Markdown: $summaryMarkdownPath" -ForegroundColor Green
Write-Host "Latest JSON: $latestJsonPath" -ForegroundColor Green
Write-Host "Latest Markdown: $latestMarkdownPath" -ForegroundColor Green

if ($anyErrors) {
	exit 4
}

if ($regressionCount -gt 0) {
	exit 5
}

if ($policyFailureCount -gt 0) {
	exit 6
}

exit 0
