param(
	[string]$RomPath,
	[string]$NexenExePath = ".\bin\win-x64\Release\Nexen.exe",
	[string]$MesenExePath = "..\Mesen2-Expanded\bin\win-x64\Release\Mesen.exe",
	[string]$NexenWorkingDir = ".",
	[string]$MesenWorkingDir = "..\Mesen2-Expanded",
	[string[]]$MesenArgs = @("--testRunner", "--timeout=35"),
	[string[]]$NexenArgs = @("--testRunner", "--timeout=35"),
	[switch]$DisableMesenFallbackRunModes,
	[switch]$AllowMissingMesenFrontend,
	[int]$AutoStopTimeoutSeconds = 45,
	[int]$FrameStart = 0,
	[int]$FrameEnd = 600,
	[int]$SnapshotFrame = 180,
	[string]$AddressStart = "0xE00000",
	[string]$AddressEnd = "0xFFFFFF",
	[int]$MaxLines = 1200000,
	[string]$ReportPath = ".\reference\cross_emu_wram_compare.txt"
)

$ErrorActionPreference = "Stop"

function Resolve-RequiredPath {
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

function Resolve-OptionalPath {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path
	)

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

function Resolve-FirstExistingPath {
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$CandidatePaths,
		[Parameter(Mandatory = $true)]
		[string]$Label
	)

	foreach ($candidate in $CandidatePaths) {
		if ([string]::IsNullOrWhiteSpace($candidate)) {
			continue
		}

		$resolvedCandidate = Resolve-OptionalPath -Path $candidate
		if ($null -ne $resolvedCandidate) {
			return $resolvedCandidate
		}
	}

	throw "Unable to resolve $Label. Tried: $($CandidatePaths -join '; ')"
}

function Start-TimedRun {
	param(
		[Parameter(Mandatory = $true)]
		[string]$ProgramPath,
		[Parameter(Mandatory = $false)]
		[string[]]$PreArgs = @(),
		[Parameter(Mandatory = $true)]
		[string]$Rom,
		[Parameter(Mandatory = $true)]
		[string]$WorkingDir,
		[Parameter(Mandatory = $true)]
		[int]$TimeoutSeconds,
		[Parameter(Mandatory = $true)]
		[string]$Name
	)

	$launchTarget = $ProgramPath
	$launchArgs = @()
	if ($PreArgs -and $PreArgs.Count -gt 0) {
		$launchArgs += $PreArgs
	}
	# Always quote ROM path explicitly so spaces/special chars are preserved.
	$quotedRom = '"' + $Rom + '"'
	$launchArgs += $quotedRom
	if ([System.IO.Path]::GetExtension($ProgramPath).Equals('.dll', [System.StringComparison]::OrdinalIgnoreCase)) {
		$launchTarget = "dotnet"
		$launchArgs = @($ProgramPath)
		if ($PreArgs -and $PreArgs.Count -gt 0) {
			$launchArgs += $PreArgs
		}
		$launchArgs += $quotedRom
	}

	Write-Host "$Name launch: $launchTarget $($launchArgs -join ' ')" -ForegroundColor DarkCyan
	$process = Start-Process -FilePath $launchTarget -WorkingDirectory $WorkingDir -ArgumentList $launchArgs -PassThru
	if ($null -eq $process) {
		throw "Failed to launch $Name process."
	}

	Write-Host "$Name started: pid=$($process.Id)" -ForegroundColor Green
	$stoppedByTimeout = $false
	$exitCode = $null
	try {
		Wait-Process -Id $process.Id -Timeout $TimeoutSeconds -ErrorAction Stop
		$process.Refresh()
		$exitCode = $process.ExitCode
		Write-Host "$Name exited before timeout." -ForegroundColor Yellow
	} catch {
		Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
		$stoppedByTimeout = $true
		Write-Host "$Name auto-stopped after $TimeoutSeconds seconds." -ForegroundColor Yellow
	}

	return [pscustomobject]@{
		Pid = $process.Id
		StoppedByTimeout = $stoppedByTimeout
		ExitCode = $exitCode
		LaunchTarget = $launchTarget
		LaunchArgs = ($launchArgs -join " ")
	}
}

function Remove-ExistingFiles {
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$Paths
	)

	foreach ($path in $Paths) {
		if ([string]::IsNullOrWhiteSpace($path)) {
			continue
		}

		if (Test-Path -LiteralPath $path) {
			Remove-Item -LiteralPath $path -Force
		}
	}
}

function Get-TraceSearchRoots {
	param(
		[string[]]$Roots
	)

	$unique = New-Object System.Collections.Generic.List[string]
	$seen = New-Object "System.Collections.Generic.HashSet[string]" ([System.StringComparer]::OrdinalIgnoreCase)

	foreach ($root in $Roots) {
		if ([string]::IsNullOrWhiteSpace($root)) {
			continue
		}

		if (!(Test-Path -LiteralPath $root)) {
			continue
		}

		$full = [System.IO.Path]::GetFullPath($root)
		if ($seen.Add($full)) {
			$unique.Add($full)
		}
	}

	return $unique
}

function Find-LatestMatchingFile {
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$Roots,
		[Parameter(Mandatory = $true)]
		[string[]]$NamePatterns,
		[int]$MaxDepth = 6
	)

	$candidates = New-Object System.Collections.Generic.List[object]
	foreach ($root in $Roots) {
		if (!(Test-Path -LiteralPath $root)) {
			continue
		}

		foreach ($pattern in $NamePatterns) {
			$files = Get-ChildItem -LiteralPath $root -Recurse -File -Filter $pattern -ErrorAction SilentlyContinue
			foreach ($file in $files) {
				$relativeSegments = ($file.FullName.Substring($root.Length).TrimStart('\\') -split '\\').Count
				if ($relativeSegments -le ($MaxDepth + 1)) {
					$candidates.Add($file)
				}
			}
		}
	}

	if ($candidates.Count -eq 0) {
		return $null
	}

	return ($candidates | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1).FullName
}

function Resolve-TraceWithFallbackSearch {
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$CandidatePaths,
		[Parameter(Mandatory = $true)]
		[string[]]$SearchRoots,
		[Parameter(Mandatory = $true)]
		[string[]]$NamePatterns,
		[Parameter(Mandatory = $true)]
		[string]$CapturePath
	)

	$direct = TryResolveExistingTracePath -CandidatePaths $CandidatePaths
	if ($null -ne $direct) {
		if ($direct -ne $CapturePath) {
			Copy-Item -LiteralPath $direct -Destination $CapturePath -Force
			return [pscustomobject]@{
				Path = $CapturePath
				SourcePath = $direct
				SourceKind = "direct-candidate-copy"
			}
		}

		return [pscustomobject]@{
			Path = $direct
			SourcePath = $direct
			SourceKind = "direct-candidate"
		}
	}

	$fallback = Find-LatestMatchingFile -Roots $SearchRoots -NamePatterns $NamePatterns
	if ($null -eq $fallback) {
		return [pscustomobject]@{
			Path = $null
			SourcePath = $null
			SourceKind = "missing"
		}
	}

	Copy-Item -LiteralPath $fallback -Destination $CapturePath -Force
	return [pscustomobject]@{
		Path = $CapturePath
		SourcePath = $fallback
		SourceKind = "fallback-search-copy"
	}
}

function Test-MesenStartupTraceEnvSupport {
	param(
		[Parameter(Mandatory = $true)]
		[string]$MesenWorkingDirectory
	)

	$sourceCandidates = @(
		(Join-Path $MesenWorkingDirectory "Core\Genesis\GenesisNativeBackend.cpp"),
		(Join-Path $MesenWorkingDirectory "..\Mesen2-Expanded\Core\Genesis\GenesisNativeBackend.cpp")
	)

	foreach ($source in $sourceCandidates) {
		if (!(Test-Path -LiteralPath $source)) {
			continue
		}

		$match = Select-String -Path $source -Pattern "MESEN_GENESIS_STARTUP_TRACE" -Quiet -ErrorAction SilentlyContinue
		if ($match) {
			return $true
		}
	}

	return $false
}

function Stop-ExistingProcessInstances {
	param(
		[Parameter(Mandatory = $true)]
		[string]$ProgramPath,
		[Parameter(Mandatory = $true)]
		[string]$Name
	)

	$processName = [System.IO.Path]::GetFileNameWithoutExtension($ProgramPath)
	if ([string]::IsNullOrWhiteSpace($processName)) {
		return
	}

	$existing = Get-Process -Name $processName -ErrorAction SilentlyContinue
	if ($null -eq $existing) {
		return
	}

	foreach ($proc in $existing) {
		try {
			Stop-Process -Id $proc.Id -Force -ErrorAction Stop
			Write-Host "Stopped existing $Name process pid=$($proc.Id)" -ForegroundColor DarkYellow
		} catch {
			Write-Warning "Failed to stop existing $Name process pid=$($proc.Id): $($_.Exception.Message)"
		}
	}
}

function Get-TraceLines {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path
	)

	if (!(Test-Path -LiteralPath $Path)) {
		throw "Trace file missing: $Path"
	}

	return Get-Content -LiteralPath $Path | Where-Object {
		$_ -and !$_.StartsWith("#")
	}
}

function Resolve-ExistingTracePath {
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$CandidatePaths,
		[Parameter(Mandatory = $true)]
		[string]$Label
	)

	foreach ($candidate in $CandidatePaths) {
		if (Test-Path -LiteralPath $candidate) {
			return $candidate
		}
	}

	throw "$Label trace file missing. Candidates: $($CandidatePaths -join '; ')"
}

function TryResolveExistingTracePath {
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$CandidatePaths
	)

	foreach ($candidate in $CandidatePaths) {
		if (Test-Path -LiteralPath $candidate) {
			return $candidate
		}
	}

	return $null
}

function Find-FirstDifference {
	param(
		[string[]]$Left,
		[string[]]$Right
	)

	$limit = [Math]::Min($Left.Count, $Right.Count)
	for ($i = 0; $i -lt $limit; $i++) {
		if ($Left[$i] -ne $Right[$i]) {
			return $i
		}
	}

	if ($Left.Count -ne $Right.Count) {
		return $limit
	}

	return -1
}

function Get-StartupEventCount {
	param(
		[string[]]$Lines,
		[string[]]$Tokens
	)

	if ($null -eq $Lines -or $null -eq $Tokens -or $Tokens.Count -eq 0) {
		return 0
	}

	$count = 0
	foreach ($line in $Lines) {
		foreach ($token in $Tokens) {
			if ([string]::IsNullOrWhiteSpace($token)) {
				continue
			}

			if ($line -like "*$token*") {
				$count++
				break
			}
		}
	}

	return $count
}

function Get-StartupMetrics {
	param(
		[string[]]$Lines
	)

	$metrics = [ordered]@{}
	$metrics["startupCheckpointCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("STARTUP_CHECKPOINT", "STARTUP_CP")
	$metrics["startupDisplayTransitionCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("VDP_DISP_TGL")
	$metrics["startupPaletteCheckpointCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("STARTUP_PAL", "STARTUP_CHECKPOINT", "STARTUP_CP")
	$metrics["startupVdpSnapshotCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("STARTUP_VDP", "STARTUP_CHECKPOINT", "STARTUP_CP")
	$metrics["startupVdpRegisterWriteCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("VDP_REG_W", "VDP_CTRL_W")
	$metrics["startupTmssUnlockCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("TMSS_UNLOCK", "TMSS_W8", "TMSS_W16")
	$metrics["startupZ80RuntimeToggleCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("Z80_RUN_TGL", "STARTUP_Z80")
	$metrics["startupZ80BusReqEventCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("Z80_BUSREQ", "STARTUP_Z80")
	$metrics["startupZ80ResetEventCount"] = Get-StartupEventCount -Lines $Lines -Tokens @("Z80_RESET", "STARTUP_Z80")

	return $metrics
}

function Get-NormalizedStartupLines {
	param(
		[string[]]$Lines
	)

	if ($null -eq $Lines -or $Lines.Count -eq 0) {
		return @()
	}

	$noiseTokens = @(" VDP_CTRL_R ", " VDP_CTRL_W ", " VDP_DATA_R ", " VDP_DATA_W ", " VDP_HV_R ")
	return @($Lines | Where-Object {
		$line = $_
		if ([string]::IsNullOrWhiteSpace($line)) {
			return $false
		}

		foreach ($token in $noiseTokens) {
			if ($line.Contains($token)) {
				return $false
			}
		}

		return $true
	})
}

function Get-StartupFrameSnapshot {
	param(
		[string[]]$Lines,
		[int]$Frame
	)

	$result = [ordered]@{}
	$result["STARTUP_PAL"] = "<missing>"
	$result["STARTUP_VDP"] = "<missing>"
	$result["VDP_REG_W"] = "<missing>"
	$result["VDP_STAT_W"] = "<missing>"
	$result["LOOP_POLL8"] = "<missing>"
	$result["LOOP_POLL16"] = "<missing>"
	$result["CPU_PRELOOP"] = "<missing>"
	$result["CPU_LOOP"] = "<missing>"

	if ($null -eq $Lines -or $Lines.Count -eq 0) {
		return $result
	}

	$framePrefix = "F{0:d4} " -f $Frame
	foreach ($line in $Lines) {
		if (-not $line.StartsWith($framePrefix)) {
			continue
		}

		if ($result["STARTUP_PAL"] -eq "<missing>" -and $line.Contains(" STARTUP_PAL ")) {
			$result["STARTUP_PAL"] = $line
			continue
		}
		if ($result["STARTUP_VDP"] -eq "<missing>" -and $line.Contains(" STARTUP_VDP ")) {
			$result["STARTUP_VDP"] = $line
			continue
		}
		if ($result["VDP_REG_W"] -eq "<missing>" -and $line.Contains(" VDP_REG_W ")) {
			$result["VDP_REG_W"] = $line
			continue
		}
		if ($result["VDP_STAT_W"] -eq "<missing>" -and $line.Contains(" VDP_STAT_W ")) {
			$result["VDP_STAT_W"] = $line
			continue
		}
		if ($result["LOOP_POLL8"] -eq "<missing>" -and $line.Contains(" LOOP_POLL8 ")) {
			$result["LOOP_POLL8"] = $line
			continue
		}
		if ($result["LOOP_POLL16"] -eq "<missing>" -and $line.Contains(" LOOP_POLL16 ")) {
			$result["LOOP_POLL16"] = $line
			continue
		}
		if ($result["CPU_PRELOOP"] -eq "<missing>" -and $line.Contains(" CPU_PRELOOP ")) {
			$result["CPU_PRELOOP"] = $line
			continue
		}
		if ($result["CPU_LOOP"] -eq "<missing>" -and $line.Contains(" CPU_LOOP ")) {
			$result["CPU_LOOP"] = $line
			continue
		}
	}

	return $result
}

function Get-StartupFrameTagStats {
	param(
		[string[]]$Lines,
		[int]$Frame,
		[string]$Tag
	)

	$result = [ordered]@{}
	$result["Count"] = 0
	$result["First"] = "<missing>"
	$result["Last"] = "<missing>"

	if ($null -eq $Lines -or $Lines.Count -eq 0 -or [string]::IsNullOrWhiteSpace($Tag)) {
		return $result
	}

	$framePrefix = "F{0:d4} " -f $Frame
	$token = " {0} " -f $Tag
	foreach ($line in $Lines) {
		if (-not $line.StartsWith($framePrefix)) {
			continue
		}
		if (-not $line.Contains($token)) {
			continue
		}

		if ($result["Count"] -eq 0) {
			$result["First"] = $line
		}
		$result["Last"] = $line
		$result["Count"] = $result["Count"] + 1
	}

	return $result
}

function Get-StartupTagGlobalStats {
	param(
		[string[]]$Lines,
		[string]$Tag
	)

	$result = [ordered]@{}
	$result["Count"] = 0
	$result["First"] = "<missing>"
	$result["Last"] = "<missing>"
	$result["FirstFrame"] = -1
	$result["LastFrame"] = -1

	if ($null -eq $Lines -or $Lines.Count -eq 0 -or [string]::IsNullOrWhiteSpace($Tag)) {
		return $result
	}

	$token = " {0} " -f $Tag
	foreach ($line in $Lines) {
		if (-not $line.Contains($token)) {
			continue
		}

		if ($result["Count"] -eq 0) {
			$result["First"] = $line
			if ($line -match '^F(\d{4}) ') {
				$result["FirstFrame"] = [int]$Matches[1]
			}
		}

		$result["Last"] = $line
		if ($line -match '^F(\d{4}) ') {
			$result["LastFrame"] = [int]$Matches[1]
		}
		$result["Count"] = $result["Count"] + 1
	}

	return $result
}

$romCandidates = @()
if (-not [string]::IsNullOrWhiteSpace($RomPath)) {
	$romCandidates += $RomPath
}
$romCandidates += @(
	"C:\~reference-roms\genesis\md\Sonic The Hedgehog (W) (REV00) [!].md",
	"C:\~reference-roms\genesis\md\Sonic The Hedgehog (W) (REV01) [!].md",
	"C:\~reference-roms\genesis\md\Sonic The Hedgehog 2 (W) (REV01) [!].md",
	"C:\~reference-roms\genesis\md\Sonic The Hedgehog (USA, Europe).md",
	"C:\~reference-roms\genesis\md\sonic\Sonic The Hedgehog (W) (REV00) [!].md",
	"C:\~reference-roms\genesis\md\sonic\Sonic The Hedgehog (W) (REV01) [!].md",
	"C:\~reference-roms\genesis\md\sonic\Sonic The Hedgehog 2 (W) (REV01) [!].md",
	"C:\~reference-roms\genesis\Sonic The Hedgehog (USA, Europe).md"
)

$mesenExeCandidates = @(
	$env:NEXEN_MESEN_FRONTEND_PATH,
	$MesenExePath,
	"..\Mesen2-Expanded\bin\win-x64\Release\Mesen.exe",
	"..\Mesen2-Expanded\bin\win-x64\Debug\Mesen.exe",
	"..\Mesen2-Expanded\bin\win-x64\Release\Mesen.dll",
	"..\Mesen2-Expanded\bin\win-x64\Debug\Mesen.dll",
	"..\Mesen2-Expanded\UI\bin\win-x64\Release\Mesen.exe",
	"..\Mesen2-Expanded\UI\bin\win-x64\Release\Mesen.dll",
	"..\Mesen2-Expanded\UI\bin\Release\Mesen.exe",
	"..\Mesen2-Expanded\UI\bin\Release\Mesen.dll",
	"..\Mesen2-Expanded\UI\bin\Release\net8.0\win-x64\publish\Mesen.exe",
	"..\Mesen2-Expanded\UI\bin\Release\net8.0\win-x64\publish\Mesen.dll"
)

$romCandidates = @($romCandidates | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
$mesenExeCandidates = @($mesenExeCandidates | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })

$resolvedRom = Resolve-FirstExistingPath -CandidatePaths $romCandidates -Label "ROM"
$resolvedNexenExe = Resolve-RequiredPath -Path $NexenExePath -Label "Nexen exe"
$resolvedMesenExe = $null
try {
	$resolvedMesenExe = Resolve-FirstExistingPath -CandidatePaths $mesenExeCandidates -Label "Mesen2-Expanded frontend exe"
} catch {
	if (-not $AllowMissingMesenFrontend) {
		throw
	}
	Write-Warning "Mesen frontend executable was not found. Continuing in Nexen-only mode."
}
$resolvedNexenDir = Resolve-RequiredPath -Path $NexenWorkingDir -Label "Nexen working dir"
$resolvedMesenDir = Resolve-RequiredPath -Path $MesenWorkingDir -Label "Mesen2-Expanded working dir"

$nexenExeDir = Split-Path -Parent $resolvedNexenExe
$mesenExeDir = $null
if ($null -ne $resolvedMesenExe) {
	$mesenExeDir = Split-Path -Parent $resolvedMesenExe
}
$mesenDocumentsDir = Join-Path ([Environment]::GetFolderPath([Environment+SpecialFolder]::MyDocuments)) "Mesen2"
$mesenAppDataDir = Join-Path ([Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData)) "Mesen2"
$nexenTraceCandidates = @(
	(Join-Path $resolvedNexenDir "reference\cpu_ram_trace.log"),
	(Join-Path $nexenExeDir "reference\cpu_ram_trace.log")
)
$mesenTraceCandidates = @((Join-Path $resolvedMesenDir "reference\cpu_ram_trace.log"))
if ($null -ne $mesenExeDir) {
	$mesenTraceCandidates += (Join-Path $mesenExeDir "reference\cpu_ram_trace.log")
}
$mesenTraceCandidates += (Join-Path $mesenDocumentsDir "reference\cpu_ram_trace.log")
$resolvedReportPath = $ReportPath
if (-not [System.IO.Path]::IsPathRooted($resolvedReportPath)) {
	$resolvedReportPath = Join-Path $resolvedNexenDir $resolvedReportPath
}
$resolvedReportPath = [System.IO.Path]::GetFullPath($resolvedReportPath)
$reportDir = Split-Path -Parent $resolvedReportPath
if (!(Test-Path -LiteralPath $reportDir)) {
	New-Item -Path $reportDir -ItemType Directory -Force | Out-Null
}

$traceRunId = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss")
$traceOutputDir = Join-Path $reportDir "trace-captures"
if (!(Test-Path -LiteralPath $traceOutputDir)) {
	New-Item -Path $traceOutputDir -ItemType Directory -Force | Out-Null
}

$nexenExplicitWramTracePath = Join-Path $traceOutputDir "nexen-cpu-ram-trace-$traceRunId.log"
$nexenExplicitStartupTracePath = Join-Path $traceOutputDir "nexen-startup-trace-$traceRunId.log"
$mesenExplicitWramTracePath = Join-Path $traceOutputDir "mesen-cpu-ram-trace-$traceRunId.log"
$mesenExplicitStartupTracePath = Join-Path $traceOutputDir "mesen-startup-trace-$traceRunId.log"

foreach ($tracePath in ($nexenTraceCandidates + $mesenTraceCandidates)) {
	if (Test-Path -LiteralPath $tracePath) {
		Remove-Item -LiteralPath $tracePath -Force
	}
}

Remove-ExistingFiles -Paths @($nexenExplicitWramTracePath, $nexenExplicitStartupTracePath, $mesenExplicitWramTracePath, $mesenExplicitStartupTracePath)

$nexenTraceCandidates = @($nexenExplicitWramTracePath) + $nexenTraceCandidates
$nexenStartupTraceCandidates = @(
	$nexenExplicitStartupTracePath,
	(Join-Path $resolvedNexenDir "reference\genesis_startup_trace.log"),
	(Join-Path $nexenExeDir "reference\genesis_startup_trace.log")
)

$mesenTraceCandidates = @($mesenExplicitWramTracePath) + $mesenTraceCandidates
$mesenStartupTraceCandidates = @(
	$mesenExplicitStartupTracePath,
	(Join-Path $resolvedMesenDir "reference\genesis_startup_trace.log")
)
if ($null -ne $mesenExeDir) {
	$mesenStartupTraceCandidates += (Join-Path $mesenExeDir "reference\genesis_startup_trace.log")
}
$mesenStartupTraceCandidates += (Join-Path $mesenDocumentsDir "reference\genesis_startup_trace.log")

$oldMesenFrameStart = $env:MESEN_WRAM_FRAME_START
$oldMesenFrameEnd = $env:MESEN_WRAM_FRAME_END
$oldMesenAddrStart = $env:MESEN_WRAM_ADDR_START
$oldMesenAddrEnd = $env:MESEN_WRAM_ADDR_END
$oldMesenMaxLines = $env:MESEN_WRAM_MAX_LINES
$oldNexenFrameStart = $env:NEXEN_WRAM_FRAME_START
$oldNexenFrameEnd = $env:NEXEN_WRAM_FRAME_END
$oldNexenAddrStart = $env:NEXEN_WRAM_ADDR_START
$oldNexenAddrEnd = $env:NEXEN_WRAM_ADDR_END
$oldNexenMaxLines = $env:NEXEN_WRAM_MAX_LINES
$oldNexenWramTracePath = $env:NEXEN_WRAM_TRACE_PATH
$oldNexenStartupTracePath = $env:NEXEN_GENESIS_STARTUP_TRACE_PATH
$oldNexenStartupTraceEnabled = $env:NEXEN_GENESIS_STARTUP_TRACE
$oldNexenStartupTraceFrameEnd = $env:NEXEN_GENESIS_STARTUP_TRACE_FRAME_END
$oldNexenStartupTraceMaxLines = $env:NEXEN_GENESIS_STARTUP_TRACE_MAX_LINES

$oldMesenWramTracePath = $env:MESEN_WRAM_TRACE_PATH
$oldMesenStartupTracePath = $env:MESEN_GENESIS_STARTUP_TRACE_PATH
$oldMesenStartupTraceEnabled = $env:MESEN_GENESIS_STARTUP_TRACE
$oldMesenStartupTraceFrameEnd = $env:MESEN_GENESIS_STARTUP_TRACE_FRAME_END
$oldMesenStartupTraceMaxLines = $env:MESEN_GENESIS_STARTUP_TRACE_MAX_LINES
$mesenRunSkipped = $false
$mesenStartupTraceEnvSupported = Test-MesenStartupTraceEnvSupport -MesenWorkingDirectory $resolvedMesenDir
$mesenAttemptNotes = New-Object System.Collections.Generic.List[string]

try {
	$env:MESEN_WRAM_FRAME_START = "$FrameStart"
	$env:MESEN_WRAM_FRAME_END = "$FrameEnd"
	$env:MESEN_WRAM_ADDR_START = "$AddressStart"
	$env:MESEN_WRAM_ADDR_END = "$AddressEnd"
	$env:MESEN_WRAM_MAX_LINES = "$MaxLines"
	$env:MESEN_WRAM_TRACE_PATH = "$mesenExplicitWramTracePath"
	$env:MESEN_GENESIS_STARTUP_TRACE_PATH = "$mesenExplicitStartupTracePath"
	$env:MESEN_GENESIS_STARTUP_TRACE = "1"
	$env:MESEN_GENESIS_STARTUP_TRACE_FRAME_END = "$FrameEnd"
	$env:MESEN_GENESIS_STARTUP_TRACE_MAX_LINES = "$MaxLines"

	$env:NEXEN_WRAM_FRAME_START = "$FrameStart"
	$env:NEXEN_WRAM_FRAME_END = "$FrameEnd"
	$env:NEXEN_WRAM_ADDR_START = "$AddressStart"
	$env:NEXEN_WRAM_ADDR_END = "$AddressEnd"
	$env:NEXEN_WRAM_MAX_LINES = "$MaxLines"
	$env:NEXEN_WRAM_TRACE_PATH = "$nexenExplicitWramTracePath"
	$env:NEXEN_GENESIS_STARTUP_TRACE_PATH = "$nexenExplicitStartupTracePath"
	$env:NEXEN_GENESIS_STARTUP_TRACE = "1"
	$env:NEXEN_GENESIS_STARTUP_TRACE_FRAME_END = "$FrameEnd"
	$env:NEXEN_GENESIS_STARTUP_TRACE_MAX_LINES = "$MaxLines"

	if ($null -ne $resolvedMesenExe) {
		Stop-ExistingProcessInstances -ProgramPath $resolvedMesenExe -Name "Mesen2-Expanded"
	}
	Stop-ExistingProcessInstances -ProgramPath $resolvedNexenExe -Name "Nexen"

	$mesenRun = [pscustomobject]@{
		Pid = 0
		StoppedByTimeout = $false
		ExitCode = $null
		LaunchTarget = ""
		LaunchArgs = ""
	}
	if ($null -ne $resolvedMesenExe) {
		$mesenRunModes = New-Object System.Collections.Generic.List[object]
		$mesenRunModes.Add([pscustomobject]@{ Name = "configured-args"; Args = $MesenArgs })
		if (-not $DisableMesenFallbackRunModes) {
			$mesenRunModes.Add([pscustomobject]@{ Name = "rom-only"; Args = @() })
		}

		$mesenTraceFound = $false
		for ($modeIndex = 0; $modeIndex -lt $mesenRunModes.Count; $modeIndex++) {
			$mode = $mesenRunModes[$modeIndex]
			Remove-ExistingFiles -Paths ($mesenTraceCandidates + $mesenStartupTraceCandidates)

			Write-Host "Running Mesen2-Expanded trace capture (mode=$($mode.Name))..." -ForegroundColor Cyan
			$mesenRun = Start-TimedRun -ProgramPath $resolvedMesenExe -PreArgs $mode.Args -Rom $resolvedRom -WorkingDir $resolvedMesenDir -TimeoutSeconds $AutoStopTimeoutSeconds -Name "Mesen2-Expanded"

			$attemptTracePath = TryResolveExistingTracePath -CandidatePaths $mesenTraceCandidates
			$attemptStartupPath = TryResolveExistingTracePath -CandidatePaths $mesenStartupTraceCandidates
			$mesenAttemptNotes.Add("attempt[$modeIndex]=$($mode.Name);trace=$attemptTracePath;startup=$attemptStartupPath;exit=$($mesenRun.ExitCode);timeout=$($mesenRun.StoppedByTimeout)")

			if ($null -ne $attemptTracePath -or $null -ne $attemptStartupPath) {
				$mesenTraceFound = $true
				break
			}
		}

		if (-not $mesenTraceFound) {
			Write-Warning "Mesen2-Expanded produced no direct trace artifacts in launch attempts; fallback search will be used."
		}
	} else {
		$mesenRunSkipped = $true
	}

	Write-Host "Running Nexen trace capture..." -ForegroundColor Cyan
	$nexenRun = Start-TimedRun -ProgramPath $resolvedNexenExe -PreArgs $NexenArgs -Rom $resolvedRom -WorkingDir $resolvedNexenDir -TimeoutSeconds $AutoStopTimeoutSeconds -Name "Nexen"
} finally {
	$env:MESEN_WRAM_FRAME_START = $oldMesenFrameStart
	$env:MESEN_WRAM_FRAME_END = $oldMesenFrameEnd
	$env:MESEN_WRAM_ADDR_START = $oldMesenAddrStart
	$env:MESEN_WRAM_ADDR_END = $oldMesenAddrEnd
	$env:MESEN_WRAM_MAX_LINES = $oldMesenMaxLines

	$env:NEXEN_WRAM_FRAME_START = $oldNexenFrameStart
	$env:NEXEN_WRAM_FRAME_END = $oldNexenFrameEnd
	$env:NEXEN_WRAM_ADDR_START = $oldNexenAddrStart
	$env:NEXEN_WRAM_ADDR_END = $oldNexenAddrEnd
	$env:NEXEN_WRAM_MAX_LINES = $oldNexenMaxLines
	$env:NEXEN_WRAM_TRACE_PATH = $oldNexenWramTracePath
	$env:NEXEN_GENESIS_STARTUP_TRACE_PATH = $oldNexenStartupTracePath
	$env:NEXEN_GENESIS_STARTUP_TRACE = $oldNexenStartupTraceEnabled
	$env:NEXEN_GENESIS_STARTUP_TRACE_FRAME_END = $oldNexenStartupTraceFrameEnd
	$env:NEXEN_GENESIS_STARTUP_TRACE_MAX_LINES = $oldNexenStartupTraceMaxLines

	$env:MESEN_WRAM_TRACE_PATH = $oldMesenWramTracePath
	$env:MESEN_GENESIS_STARTUP_TRACE_PATH = $oldMesenStartupTracePath
	$env:MESEN_GENESIS_STARTUP_TRACE = $oldMesenStartupTraceEnabled
	$env:MESEN_GENESIS_STARTUP_TRACE_FRAME_END = $oldMesenStartupTraceFrameEnd
	$env:MESEN_GENESIS_STARTUP_TRACE_MAX_LINES = $oldMesenStartupTraceMaxLines
}

$mesenSearchRoots = Get-TraceSearchRoots -Roots @($resolvedMesenDir, $mesenExeDir, $mesenDocumentsDir, $mesenAppDataDir)
$nexenSearchRoots = Get-TraceSearchRoots -Roots @($resolvedNexenDir, $nexenExeDir)

$mesenTraceResult = Resolve-TraceWithFallbackSearch -CandidatePaths $mesenTraceCandidates -SearchRoots $mesenSearchRoots -NamePatterns @("cpu_ram_trace.log", "mesen-cpu-ram-trace-*.log") -CapturePath $mesenExplicitWramTracePath
$nexenTraceResult = Resolve-TraceWithFallbackSearch -CandidatePaths $nexenTraceCandidates -SearchRoots $nexenSearchRoots -NamePatterns @("cpu_ram_trace.log", "nexen-cpu-ram-trace-*.log") -CapturePath $nexenExplicitWramTracePath

$mesenTracePath = $mesenTraceResult.Path
$nexenTracePath = $nexenTraceResult.Path

$mesenStartupTraceResult = Resolve-TraceWithFallbackSearch -CandidatePaths $mesenStartupTraceCandidates -SearchRoots $mesenSearchRoots -NamePatterns @("genesis_startup_trace.log", "mesen-startup-trace-*.log") -CapturePath $mesenExplicitStartupTracePath
$nexenStartupTraceResult = Resolve-TraceWithFallbackSearch -CandidatePaths $nexenStartupTraceCandidates -SearchRoots $nexenSearchRoots -NamePatterns @("genesis_startup_trace.log", "nexen-startup-trace-*.log") -CapturePath $nexenExplicitStartupTracePath

$mesenStartupTracePath = $mesenStartupTraceResult.Path
$nexenStartupTracePath = $nexenStartupTraceResult.Path

$mesenLines = @()
$nexenLines = @()
$firstDiff = -1
$mesenHash = "missing"
$nexenHash = "missing"
$mesenStartupLines = @()
$nexenStartupLines = @()
$mesenStartupHash = "missing"
$nexenStartupHash = "missing"
$startupFirstDiff = -1
$mesenStartupCheckpointCount = 0
$nexenStartupCheckpointCount = 0
$mesenStartupDisplayTransitionCount = 0
$nexenStartupDisplayTransitionCount = 0
$mesenStartupPaletteCheckpointCount = 0
$nexenStartupPaletteCheckpointCount = 0
$mesenStartupVdpSnapshotCount = 0
$nexenStartupVdpSnapshotCount = 0
$mesenStartupVdpRegisterWriteCount = 0
$nexenStartupVdpRegisterWriteCount = 0
$mesenStartupTmssUnlockCount = 0
$nexenStartupTmssUnlockCount = 0
$mesenStartupZ80RuntimeToggleCount = 0
$nexenStartupZ80RuntimeToggleCount = 0
$mesenStartupZ80BusReqEventCount = 0
$nexenStartupZ80BusReqEventCount = 0
$mesenStartupZ80ResetEventCount = 0
$nexenStartupZ80ResetEventCount = 0
$mesenStartupNormalizedLines = @()
$nexenStartupNormalizedLines = @()
$startupNormalizedFirstDiff = -1
$mesenFrameSnapshot = [ordered]@{}
$nexenFrameSnapshot = [ordered]@{}

if ($null -ne $mesenTracePath) {
	$mesenLines = Get-TraceLines -Path $mesenTracePath
	$mesenHash = (Get-FileHash -LiteralPath $mesenTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($null -ne $nexenTracePath) {
	$nexenLines = Get-TraceLines -Path $nexenTracePath
	$nexenHash = (Get-FileHash -LiteralPath $nexenTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($null -ne $mesenStartupTracePath) {
	$mesenStartupLines = Get-TraceLines -Path $mesenStartupTracePath
	$mesenStartupHash = (Get-FileHash -LiteralPath $mesenStartupTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($null -ne $nexenStartupTracePath) {
	$nexenStartupLines = Get-TraceLines -Path $nexenStartupTracePath
	$nexenStartupHash = (Get-FileHash -LiteralPath $nexenStartupTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($mesenLines.Count -gt 0 -or $nexenLines.Count -gt 0) {
	$firstDiff = Find-FirstDifference -Left $mesenLines -Right $nexenLines
}

if ($mesenStartupLines.Count -gt 0 -or $nexenStartupLines.Count -gt 0) {
	$startupFirstDiff = Find-FirstDifference -Left $mesenStartupLines -Right $nexenStartupLines
	$mesenStartupNormalizedLines = Get-NormalizedStartupLines -Lines $mesenStartupLines
	$nexenStartupNormalizedLines = Get-NormalizedStartupLines -Lines $nexenStartupLines
	$startupNormalizedFirstDiff = Find-FirstDifference -Left $mesenStartupNormalizedLines -Right $nexenStartupNormalizedLines
	$mesenStartupMetrics = Get-StartupMetrics -Lines $mesenStartupLines
	$nexenStartupMetrics = Get-StartupMetrics -Lines $nexenStartupLines
	$mesenFrameSnapshot = Get-StartupFrameSnapshot -Lines $mesenStartupLines -Frame $SnapshotFrame
	$nexenFrameSnapshot = Get-StartupFrameSnapshot -Lines $nexenStartupLines -Frame $SnapshotFrame
	$mesenLoopPoll16Stats = Get-StartupFrameTagStats -Lines $mesenStartupLines -Frame $SnapshotFrame -Tag "LOOP_POLL16"
	$nexenLoopPoll16Stats = Get-StartupFrameTagStats -Lines $nexenStartupLines -Frame $SnapshotFrame -Tag "LOOP_POLL16"
	$mesenCpuPreloopStats = Get-StartupFrameTagStats -Lines $mesenStartupLines -Frame $SnapshotFrame -Tag "CPU_PRELOOP"
	$nexenCpuPreloopStats = Get-StartupFrameTagStats -Lines $nexenStartupLines -Frame $SnapshotFrame -Tag "CPU_PRELOOP"
	$mesenCpuPreloopGlobalStats = Get-StartupTagGlobalStats -Lines $mesenStartupLines -Tag "CPU_PRELOOP"
	$nexenCpuPreloopGlobalStats = Get-StartupTagGlobalStats -Lines $nexenStartupLines -Tag "CPU_PRELOOP"
	$mesenCpuLoopStats = Get-StartupFrameTagStats -Lines $mesenStartupLines -Frame $SnapshotFrame -Tag "CPU_LOOP"
	$nexenCpuLoopStats = Get-StartupFrameTagStats -Lines $nexenStartupLines -Frame $SnapshotFrame -Tag "CPU_LOOP"

	$mesenStartupCheckpointCount = $mesenStartupMetrics.startupCheckpointCount
	$nexenStartupCheckpointCount = $nexenStartupMetrics.startupCheckpointCount
	$mesenStartupDisplayTransitionCount = $mesenStartupMetrics.startupDisplayTransitionCount
	$nexenStartupDisplayTransitionCount = $nexenStartupMetrics.startupDisplayTransitionCount
	$mesenStartupPaletteCheckpointCount = $mesenStartupMetrics.startupPaletteCheckpointCount
	$nexenStartupPaletteCheckpointCount = $nexenStartupMetrics.startupPaletteCheckpointCount
	$mesenStartupVdpSnapshotCount = $mesenStartupMetrics.startupVdpSnapshotCount
	$nexenStartupVdpSnapshotCount = $nexenStartupMetrics.startupVdpSnapshotCount
	$mesenStartupVdpRegisterWriteCount = $mesenStartupMetrics.startupVdpRegisterWriteCount
	$nexenStartupVdpRegisterWriteCount = $nexenStartupMetrics.startupVdpRegisterWriteCount
	$mesenStartupTmssUnlockCount = $mesenStartupMetrics.startupTmssUnlockCount
	$nexenStartupTmssUnlockCount = $nexenStartupMetrics.startupTmssUnlockCount
	$mesenStartupZ80RuntimeToggleCount = $mesenStartupMetrics.startupZ80RuntimeToggleCount
	$nexenStartupZ80RuntimeToggleCount = $nexenStartupMetrics.startupZ80RuntimeToggleCount
	$mesenStartupZ80BusReqEventCount = $mesenStartupMetrics.startupZ80BusReqEventCount
	$nexenStartupZ80BusReqEventCount = $nexenStartupMetrics.startupZ80BusReqEventCount
	$mesenStartupZ80ResetEventCount = $mesenStartupMetrics.startupZ80ResetEventCount
	$nexenStartupZ80ResetEventCount = $nexenStartupMetrics.startupZ80ResetEventCount
}

$report = New-Object System.Collections.Generic.List[string]
$report.Add("GENESIS_CROSS_EMU_WRAM_TRACE_COMPARE")
$report.Add("rom=$resolvedRom")
$report.Add("frameStart=$FrameStart")
$report.Add("frameEnd=$FrameEnd")
$report.Add("autoStopTimeoutSeconds=$AutoStopTimeoutSeconds")
$report.Add("maxLines=$MaxLines")
$report.Add("mesenTrace=$mesenTracePath")
$report.Add("nexenTrace=$nexenTracePath")
$report.Add("mesenStartupTrace=$mesenStartupTracePath")
$report.Add("nexenStartupTrace=$nexenStartupTracePath")
$report.Add("mesenTraceSourceKind=$($mesenTraceResult.SourceKind)")
$report.Add("nexenTraceSourceKind=$($nexenTraceResult.SourceKind)")
$report.Add("mesenStartupTraceSourceKind=$($mesenStartupTraceResult.SourceKind)")
$report.Add("nexenStartupTraceSourceKind=$($nexenStartupTraceResult.SourceKind)")
$report.Add("mesenTraceSourcePath=$($mesenTraceResult.SourcePath)")
$report.Add("nexenTraceSourcePath=$($nexenTraceResult.SourcePath)")
$report.Add("mesenStartupTraceSourcePath=$($mesenStartupTraceResult.SourcePath)")
$report.Add("nexenStartupTraceSourcePath=$($nexenStartupTraceResult.SourcePath)")
$report.Add("mesenLines=$($mesenLines.Count)")
$report.Add("nexenLines=$($nexenLines.Count)")
$report.Add("mesenStartupLines=$($mesenStartupLines.Count)")
$report.Add("nexenStartupLines=$($nexenStartupLines.Count)")
$report.Add("snapshotFrame=$SnapshotFrame")
$report.Add("mesenStartupNormalizedLines=$($mesenStartupNormalizedLines.Count)")
$report.Add("nexenStartupNormalizedLines=$($nexenStartupNormalizedLines.Count)")
$report.Add("mesenStartupCheckpointCount=$mesenStartupCheckpointCount")
$report.Add("nexenStartupCheckpointCount=$nexenStartupCheckpointCount")
$report.Add("mesenStartupDisplayTransitionCount=$mesenStartupDisplayTransitionCount")
$report.Add("nexenStartupDisplayTransitionCount=$nexenStartupDisplayTransitionCount")
$report.Add("mesenStartupPaletteCheckpointCount=$mesenStartupPaletteCheckpointCount")
$report.Add("nexenStartupPaletteCheckpointCount=$nexenStartupPaletteCheckpointCount")
$report.Add("mesenStartupVdpSnapshotCount=$mesenStartupVdpSnapshotCount")
$report.Add("nexenStartupVdpSnapshotCount=$nexenStartupVdpSnapshotCount")
$report.Add("mesenStartupVdpRegisterWriteCount=$mesenStartupVdpRegisterWriteCount")
$report.Add("nexenStartupVdpRegisterWriteCount=$nexenStartupVdpRegisterWriteCount")
$report.Add("mesenStartupTmssUnlockCount=$mesenStartupTmssUnlockCount")
$report.Add("nexenStartupTmssUnlockCount=$nexenStartupTmssUnlockCount")
$report.Add("mesenStartupZ80RuntimeToggleCount=$mesenStartupZ80RuntimeToggleCount")
$report.Add("nexenStartupZ80RuntimeToggleCount=$nexenStartupZ80RuntimeToggleCount")
$report.Add("mesenStartupZ80BusReqEventCount=$mesenStartupZ80BusReqEventCount")
$report.Add("nexenStartupZ80BusReqEventCount=$nexenStartupZ80BusReqEventCount")
$report.Add("mesenStartupZ80ResetEventCount=$mesenStartupZ80ResetEventCount")
$report.Add("nexenStartupZ80ResetEventCount=$nexenStartupZ80ResetEventCount")
$report.Add("mesenHash=$mesenHash")
$report.Add("nexenHash=$nexenHash")
$report.Add("mesenStartupHash=$mesenStartupHash")
$report.Add("nexenStartupHash=$nexenStartupHash")
$report.Add("mesenFrontendMissing=$($null -eq $resolvedMesenExe)")
$report.Add("mesenRunSkipped=$mesenRunSkipped")
$report.Add("mesenTraceMissing=$($null -eq $mesenTracePath)")
$report.Add("nexenTraceMissing=$($null -eq $nexenTracePath)")
$report.Add("mesenStartupTraceMissing=$($null -eq $mesenStartupTracePath)")
$report.Add("nexenStartupTraceMissing=$($null -eq $nexenStartupTracePath)")
$report.Add("mesenTimedOut=$($mesenRun.StoppedByTimeout)")
$report.Add("nexenTimedOut=$($nexenRun.StoppedByTimeout)")
$report.Add("mesenExitCode=$($mesenRun.ExitCode)")
$report.Add("nexenExitCode=$($nexenRun.ExitCode)")
$report.Add("mesenLaunchTarget=$($mesenRun.LaunchTarget)")
$report.Add("mesenLaunchArgs=$($mesenRun.LaunchArgs)")
$report.Add("nexenLaunchTarget=$($nexenRun.LaunchTarget)")
$report.Add("nexenLaunchArgs=$($nexenRun.LaunchArgs)")
$report.Add("mesenStartupTraceEnvSupported=$mesenStartupTraceEnvSupported")
$report.Add("mesenSearchRoots=$($mesenSearchRoots -join ';')")
$report.Add("nexenSearchRoots=$($nexenSearchRoots -join ';')")
$report.Add("mesenTraceCandidates=$($mesenTraceCandidates -join ';')")
$report.Add("mesenStartupTraceCandidates=$($mesenStartupTraceCandidates -join ';')")
$report.Add("nexenTraceCandidates=$($nexenTraceCandidates -join ';')")
$report.Add("nexenStartupTraceCandidates=$($nexenStartupTraceCandidates -join ';')")
if ($mesenAttemptNotes.Count -gt 0) {
	$report.Add("mesenAttempts=$($mesenAttemptNotes -join ' | ')")
}
$report.Add("firstDiffIndex=$firstDiff")
$report.Add("startupFirstDiffIndex=$startupFirstDiff")
$report.Add("startupNormalizedFirstDiffIndex=$startupNormalizedFirstDiff")

if ($firstDiff -ge 0) {
	$mesenFirst = if ($firstDiff -lt $mesenLines.Count) { $mesenLines[$firstDiff] } else { "<no-line>" }
	$nexenFirst = if ($firstDiff -lt $nexenLines.Count) { $nexenLines[$firstDiff] } else { "<no-line>" }
	$report.Add("mesenFirstDiff=$mesenFirst")
	$report.Add("nexenFirstDiff=$nexenFirst")
} else {
	$report.Add("status=identical")
}

if ($startupFirstDiff -ge 0) {
	$mesenStartupFirst = if ($startupFirstDiff -lt $mesenStartupLines.Count) { $mesenStartupLines[$startupFirstDiff] } else { "<no-line>" }
	$nexenStartupFirst = if ($startupFirstDiff -lt $nexenStartupLines.Count) { $nexenStartupLines[$startupFirstDiff] } else { "<no-line>" }
	$report.Add("mesenStartupFirstDiff=$mesenStartupFirst")
	$report.Add("nexenStartupFirstDiff=$nexenStartupFirst")
}

if ($startupNormalizedFirstDiff -ge 0) {
	$mesenStartupNormalizedFirst = if ($startupNormalizedFirstDiff -lt $mesenStartupNormalizedLines.Count) { $mesenStartupNormalizedLines[$startupNormalizedFirstDiff] } else { "<no-line>" }
	$nexenStartupNormalizedFirst = if ($startupNormalizedFirstDiff -lt $nexenStartupNormalizedLines.Count) { $nexenStartupNormalizedLines[$startupNormalizedFirstDiff] } else { "<no-line>" }
	$report.Add("mesenStartupNormalizedFirstDiff=$mesenStartupNormalizedFirst")
	$report.Add("nexenStartupNormalizedFirstDiff=$nexenStartupNormalizedFirst")
}

$report.Add("mesenSnapshot_STARTUP_PAL=$($mesenFrameSnapshot.STARTUP_PAL)")
$report.Add("nexenSnapshot_STARTUP_PAL=$($nexenFrameSnapshot.STARTUP_PAL)")
$report.Add("mesenSnapshot_STARTUP_VDP=$($mesenFrameSnapshot.STARTUP_VDP)")
$report.Add("nexenSnapshot_STARTUP_VDP=$($nexenFrameSnapshot.STARTUP_VDP)")
$report.Add("mesenSnapshot_VDP_REG_W=$($mesenFrameSnapshot.VDP_REG_W)")
$report.Add("nexenSnapshot_VDP_REG_W=$($nexenFrameSnapshot.VDP_REG_W)")
$report.Add("mesenSnapshot_VDP_STAT_W=$($mesenFrameSnapshot.VDP_STAT_W)")
$report.Add("nexenSnapshot_VDP_STAT_W=$($nexenFrameSnapshot.VDP_STAT_W)")
$report.Add("mesenSnapshot_LOOP_POLL8=$($mesenFrameSnapshot.LOOP_POLL8)")
$report.Add("nexenSnapshot_LOOP_POLL8=$($nexenFrameSnapshot.LOOP_POLL8)")
$report.Add("mesenSnapshot_LOOP_POLL16=$($mesenFrameSnapshot.LOOP_POLL16)")
$report.Add("nexenSnapshot_LOOP_POLL16=$($nexenFrameSnapshot.LOOP_POLL16)")
$report.Add("mesenSnapshot_CPU_PRELOOP=$($mesenFrameSnapshot.CPU_PRELOOP)")
$report.Add("nexenSnapshot_CPU_PRELOOP=$($nexenFrameSnapshot.CPU_PRELOOP)")
$report.Add("mesenSnapshot_CPU_LOOP=$($mesenFrameSnapshot.CPU_LOOP)")
$report.Add("nexenSnapshot_CPU_LOOP=$($nexenFrameSnapshot.CPU_LOOP)")
$report.Add("mesenLoopPoll16Count=$($mesenLoopPoll16Stats.Count)")
$report.Add("nexenLoopPoll16Count=$($nexenLoopPoll16Stats.Count)")
$report.Add("mesenLoopPoll16First=$($mesenLoopPoll16Stats.First)")
$report.Add("nexenLoopPoll16First=$($nexenLoopPoll16Stats.First)")
$report.Add("mesenLoopPoll16Last=$($mesenLoopPoll16Stats.Last)")
$report.Add("nexenLoopPoll16Last=$($nexenLoopPoll16Stats.Last)")
$report.Add("mesenCpuPreloopCount=$($mesenCpuPreloopStats.Count)")
$report.Add("nexenCpuPreloopCount=$($nexenCpuPreloopStats.Count)")
$report.Add("mesenCpuPreloopFirst=$($mesenCpuPreloopStats.First)")
$report.Add("nexenCpuPreloopFirst=$($nexenCpuPreloopStats.First)")
$report.Add("mesenCpuPreloopLast=$($mesenCpuPreloopStats.Last)")
$report.Add("nexenCpuPreloopLast=$($nexenCpuPreloopStats.Last)")
$report.Add("mesenCpuPreloopGlobalCount=$($mesenCpuPreloopGlobalStats.Count)")
$report.Add("nexenCpuPreloopGlobalCount=$($nexenCpuPreloopGlobalStats.Count)")
$report.Add("mesenCpuPreloopGlobalFirstFrame=$($mesenCpuPreloopGlobalStats.FirstFrame)")
$report.Add("nexenCpuPreloopGlobalFirstFrame=$($nexenCpuPreloopGlobalStats.FirstFrame)")
$report.Add("mesenCpuPreloopGlobalLastFrame=$($mesenCpuPreloopGlobalStats.LastFrame)")
$report.Add("nexenCpuPreloopGlobalLastFrame=$($nexenCpuPreloopGlobalStats.LastFrame)")
$report.Add("mesenCpuPreloopGlobalFirst=$($mesenCpuPreloopGlobalStats.First)")
$report.Add("nexenCpuPreloopGlobalFirst=$($nexenCpuPreloopGlobalStats.First)")
$report.Add("mesenCpuPreloopGlobalLast=$($mesenCpuPreloopGlobalStats.Last)")
$report.Add("nexenCpuPreloopGlobalLast=$($nexenCpuPreloopGlobalStats.Last)")
$report.Add("mesenCpuLoopCount=$($mesenCpuLoopStats.Count)")
$report.Add("nexenCpuLoopCount=$($nexenCpuLoopStats.Count)")
$report.Add("mesenCpuLoopFirst=$($mesenCpuLoopStats.First)")
$report.Add("nexenCpuLoopFirst=$($nexenCpuLoopStats.First)")
$report.Add("mesenCpuLoopLast=$($mesenCpuLoopStats.Last)")
$report.Add("nexenCpuLoopLast=$($nexenCpuLoopStats.Last)")

$report | Out-File -LiteralPath $resolvedReportPath -Encoding utf8

Write-Host "Trace comparison report: $resolvedReportPath" -ForegroundColor Green
Write-Host "mesenLines=$($mesenLines.Count) nexenLines=$($nexenLines.Count) firstDiffIndex=$firstDiff" -ForegroundColor Green
if ($null -eq $mesenTracePath) {
	Write-Warning "Mesen2-Expanded trace file was not emitted."
}
if ($null -eq $nexenTracePath) {
	Write-Warning "Nexen trace file was not emitted."
}
if ($null -eq $mesenStartupTracePath) {
	if (-not $mesenStartupTraceEnvSupported) {
		Write-Warning "Mesen2-Expanded startup trace file was not emitted (startup trace env support not detected in source tree)."
	} else {
		Write-Warning "Mesen2-Expanded startup trace file was not emitted."
	}
}
if ($null -eq $nexenStartupTracePath) {
	Write-Warning "Nexen startup trace file was not emitted."
}
if ($firstDiff -ge 0) {
	Write-Host "First difference found at index $firstDiff" -ForegroundColor Yellow
	exit 3
}

if ($mesenRunSkipped) {
	Write-Warning "Mesen2-Expanded run was skipped because no frontend executable was found."
	exit 0
}

exit 0
