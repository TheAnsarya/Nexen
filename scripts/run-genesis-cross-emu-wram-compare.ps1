param(
	[string]$RomPath,
	[string]$NexenExePath = ".\bin\win-x64\Release\Nexen.exe",
	[string]$NexenRefExePath = "..\Mesen2-Expanded\bin\win-x64\Release\Mesen.exe",
	[string]$NexenWorkingDir = ".",
	[string]$NexenRefWorkingDir = "..\Mesen2-Expanded",
	[string[]]$NexenRefArgs = @("--testRunner", "--timeout=35"),
	[string[]]$NexenArgs = @("--testRunner", "--timeout=35"),
	[switch]$DisableNexenRefFallbackRunModes,
	[switch]$AllowMissingNexenRefFrontend,
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

function Test-NexenRefStartupTraceEnvSupport {
	param(
		[Parameter(Mandatory = $true)]
		[string]$NexenRefWorkingDirectory
	)

	$sourceCandidates = @(
		(Join-Path $NexenRefWorkingDirectory "Core\Genesis\GenesisNativeBackend.cpp"),
		(Join-Path $NexenRefWorkingDirectory "..\Mesen2-Expanded\Core\Genesis\GenesisNativeBackend.cpp")
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

$nexenRefExeCandidates = @(
	$env:NEXEN_MESEN_FRONTEND_PATH,
	$NexenRefExePath,
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
$nexenRefExeCandidates = @($nexenRefExeCandidates | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })

$resolvedRom = Resolve-FirstExistingPath -CandidatePaths $romCandidates -Label "ROM"
$resolvedNexenExe = Resolve-RequiredPath -Path $NexenExePath -Label "Nexen exe"
$resolvedNexenRefExe = $null
try {
	$resolvedNexenRefExe = Resolve-FirstExistingPath -CandidatePaths $nexenRefExeCandidates -Label "NexenRef frontend exe"
} catch {
	if (-not $AllowMissingNexenRefFrontend) {
		throw
	}
	Write-Warning "NexenRef frontend executable was not found. Continuing in Nexen-only mode."
}
$resolvedNexenDir = Resolve-RequiredPath -Path $NexenWorkingDir -Label "Nexen working dir"
$resolvedNexenRefDir = Resolve-RequiredPath -Path $NexenRefWorkingDir -Label "NexenRef working dir"

$nexenExeDir = Split-Path -Parent $resolvedNexenExe
$nexenRefExeDir = $null
if ($null -ne $resolvedNexenRefExe) {
	$nexenRefExeDir = Split-Path -Parent $resolvedNexenRefExe
}
$nexenRefDocumentsDir = Join-Path ([Environment]::GetFolderPath([Environment+SpecialFolder]::MyDocuments)) "Mesen2"
$nexenRefAppDataDir = Join-Path ([Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData)) "Mesen2"
$nexenTraceCandidates = @(
	(Join-Path $resolvedNexenDir "reference\cpu_ram_trace.log"),
	(Join-Path $nexenExeDir "reference\cpu_ram_trace.log")
)
$nexenRefTraceCandidates = @((Join-Path $resolvedNexenRefDir "reference\cpu_ram_trace.log"))
if ($null -ne $nexenRefExeDir) {
	$nexenRefTraceCandidates += (Join-Path $nexenRefExeDir "reference\cpu_ram_trace.log")
}
$nexenRefTraceCandidates += (Join-Path $nexenRefDocumentsDir "reference\cpu_ram_trace.log")
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
$nexenRefExplicitWramTracePath = Join-Path $traceOutputDir "nexenRef-cpu-ram-trace-$traceRunId.log"
$nexenRefExplicitStartupTracePath = Join-Path $traceOutputDir "nexenRef-startup-trace-$traceRunId.log"

foreach ($tracePath in ($nexenTraceCandidates + $nexenRefTraceCandidates)) {
	if (Test-Path -LiteralPath $tracePath) {
		Remove-Item -LiteralPath $tracePath -Force
	}
}

Remove-ExistingFiles -Paths @($nexenExplicitWramTracePath, $nexenExplicitStartupTracePath, $nexenRefExplicitWramTracePath, $nexenRefExplicitStartupTracePath)

$nexenTraceCandidates = @($nexenExplicitWramTracePath) + $nexenTraceCandidates
$nexenStartupTraceCandidates = @(
	$nexenExplicitStartupTracePath,
	(Join-Path $resolvedNexenDir "reference\genesis_startup_trace.log"),
	(Join-Path $nexenExeDir "reference\genesis_startup_trace.log")
)

$nexenRefTraceCandidates = @($nexenRefExplicitWramTracePath) + $nexenRefTraceCandidates
$nexenRefStartupTraceCandidates = @(
	$nexenRefExplicitStartupTracePath,
	(Join-Path $resolvedNexenRefDir "reference\genesis_startup_trace.log")
)
if ($null -ne $nexenRefExeDir) {
	$nexenRefStartupTraceCandidates += (Join-Path $nexenRefExeDir "reference\genesis_startup_trace.log")
}
$nexenRefStartupTraceCandidates += (Join-Path $nexenRefDocumentsDir "reference\genesis_startup_trace.log")

$oldNexenRefFrameStart = $env:MESEN_WRAM_FRAME_START
$oldNexenRefFrameEnd = $env:MESEN_WRAM_FRAME_END
$oldNexenRefAddrStart = $env:MESEN_WRAM_ADDR_START
$oldNexenRefAddrEnd = $env:MESEN_WRAM_ADDR_END
$oldNexenRefMaxLines = $env:MESEN_WRAM_MAX_LINES
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

$oldNexenRefWramTracePath = $env:MESEN_WRAM_TRACE_PATH
$oldNexenRefStartupTracePath = $env:MESEN_GENESIS_STARTUP_TRACE_PATH
$oldNexenRefStartupTraceEnabled = $env:MESEN_GENESIS_STARTUP_TRACE
$oldNexenRefStartupTraceFrameEnd = $env:MESEN_GENESIS_STARTUP_TRACE_FRAME_END
$oldNexenRefStartupTraceMaxLines = $env:MESEN_GENESIS_STARTUP_TRACE_MAX_LINES
$nexenRefRunSkipped = $false
$nexenRefStartupTraceEnvSupported = Test-NexenRefStartupTraceEnvSupport -NexenRefWorkingDirectory $resolvedNexenRefDir
$nexenRefAttemptNotes = New-Object System.Collections.Generic.List[string]

try {
	$env:MESEN_WRAM_FRAME_START = "$FrameStart"
	$env:MESEN_WRAM_FRAME_END = "$FrameEnd"
	$env:MESEN_WRAM_ADDR_START = "$AddressStart"
	$env:MESEN_WRAM_ADDR_END = "$AddressEnd"
	$env:MESEN_WRAM_MAX_LINES = "$MaxLines"
	$env:MESEN_WRAM_TRACE_PATH = "$nexenRefExplicitWramTracePath"
	$env:MESEN_GENESIS_STARTUP_TRACE_PATH = "$nexenRefExplicitStartupTracePath"
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

	if ($null -ne $resolvedNexenRefExe) {
		Stop-ExistingProcessInstances -ProgramPath $resolvedNexenRefExe -Name "NexenRef"
	}
	Stop-ExistingProcessInstances -ProgramPath $resolvedNexenExe -Name "Nexen"

	$nexenRefRun = [pscustomobject]@{
		Pid = 0
		StoppedByTimeout = $false
		ExitCode = $null
		LaunchTarget = ""
		LaunchArgs = ""
	}
	if ($null -ne $resolvedNexenRefExe) {
		$nexenRefRunModes = New-Object System.Collections.Generic.List[object]
		$nexenRefRunModes.Add([pscustomobject]@{ Name = "configured-args"; Args = $NexenRefArgs })
		if (-not $DisableNexenRefFallbackRunModes) {
			$nexenRefRunModes.Add([pscustomobject]@{ Name = "rom-only"; Args = @() })
		}

		$nexenRefTraceFound = $false
		for ($modeIndex = 0; $modeIndex -lt $nexenRefRunModes.Count; $modeIndex++) {
			$mode = $nexenRefRunModes[$modeIndex]
			Remove-ExistingFiles -Paths ($nexenRefTraceCandidates + $nexenRefStartupTraceCandidates)

			Write-Host "Running NexenRef trace capture (mode=$($mode.Name))..." -ForegroundColor Cyan
			$nexenRefRun = Start-TimedRun -ProgramPath $resolvedNexenRefExe -PreArgs $mode.Args -Rom $resolvedRom -WorkingDir $resolvedNexenRefDir -TimeoutSeconds $AutoStopTimeoutSeconds -Name "NexenRef"

			$attemptTracePath = TryResolveExistingTracePath -CandidatePaths $nexenRefTraceCandidates
			$attemptStartupPath = TryResolveExistingTracePath -CandidatePaths $nexenRefStartupTraceCandidates
			$nexenRefAttemptNotes.Add("attempt[$modeIndex]=$($mode.Name);trace=$attemptTracePath;startup=$attemptStartupPath;exit=$($nexenRefRun.ExitCode);timeout=$($nexenRefRun.StoppedByTimeout)")

			if ($null -ne $attemptTracePath -or $null -ne $attemptStartupPath) {
				$nexenRefTraceFound = $true
				break
			}
		}

		if (-not $nexenRefTraceFound) {
			Write-Warning "NexenRef produced no direct trace artifacts in launch attempts; fallback search will be used."
		}
	} else {
		$nexenRefRunSkipped = $true
	}

	Write-Host "Running Nexen trace capture..." -ForegroundColor Cyan
	$nexenRun = Start-TimedRun -ProgramPath $resolvedNexenExe -PreArgs $NexenArgs -Rom $resolvedRom -WorkingDir $resolvedNexenDir -TimeoutSeconds $AutoStopTimeoutSeconds -Name "Nexen"
} finally {
	$env:MESEN_WRAM_FRAME_START = $oldNexenRefFrameStart
	$env:MESEN_WRAM_FRAME_END = $oldNexenRefFrameEnd
	$env:MESEN_WRAM_ADDR_START = $oldNexenRefAddrStart
	$env:MESEN_WRAM_ADDR_END = $oldNexenRefAddrEnd
	$env:MESEN_WRAM_MAX_LINES = $oldNexenRefMaxLines

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

	$env:MESEN_WRAM_TRACE_PATH = $oldNexenRefWramTracePath
	$env:MESEN_GENESIS_STARTUP_TRACE_PATH = $oldNexenRefStartupTracePath
	$env:MESEN_GENESIS_STARTUP_TRACE = $oldNexenRefStartupTraceEnabled
	$env:MESEN_GENESIS_STARTUP_TRACE_FRAME_END = $oldNexenRefStartupTraceFrameEnd
	$env:MESEN_GENESIS_STARTUP_TRACE_MAX_LINES = $oldNexenRefStartupTraceMaxLines
}

$nexenRefSearchRoots = Get-TraceSearchRoots -Roots @($resolvedNexenRefDir, $nexenRefExeDir, $nexenRefDocumentsDir, $nexenRefAppDataDir)
$nexenSearchRoots = Get-TraceSearchRoots -Roots @($resolvedNexenDir, $nexenExeDir)

$nexenRefTraceResult = Resolve-TraceWithFallbackSearch -CandidatePaths $nexenRefTraceCandidates -SearchRoots $nexenRefSearchRoots -NamePatterns @("cpu_ram_trace.log", "nexenRef-cpu-ram-trace-*.log") -CapturePath $nexenRefExplicitWramTracePath
$nexenTraceResult = Resolve-TraceWithFallbackSearch -CandidatePaths $nexenTraceCandidates -SearchRoots $nexenSearchRoots -NamePatterns @("cpu_ram_trace.log", "nexen-cpu-ram-trace-*.log") -CapturePath $nexenExplicitWramTracePath

$nexenRefTracePath = $nexenRefTraceResult.Path
$nexenTracePath = $nexenTraceResult.Path

$nexenRefStartupTraceResult = Resolve-TraceWithFallbackSearch -CandidatePaths $nexenRefStartupTraceCandidates -SearchRoots $nexenRefSearchRoots -NamePatterns @("genesis_startup_trace.log", "nexenRef-startup-trace-*.log") -CapturePath $nexenRefExplicitStartupTracePath
$nexenStartupTraceResult = Resolve-TraceWithFallbackSearch -CandidatePaths $nexenStartupTraceCandidates -SearchRoots $nexenSearchRoots -NamePatterns @("genesis_startup_trace.log", "nexen-startup-trace-*.log") -CapturePath $nexenExplicitStartupTracePath

$nexenRefStartupTracePath = $nexenRefStartupTraceResult.Path
$nexenStartupTracePath = $nexenStartupTraceResult.Path

$nexenRefLines = @()
$nexenLines = @()
$firstDiff = -1
$nexenRefHash = "missing"
$nexenHash = "missing"
$nexenRefStartupLines = @()
$nexenStartupLines = @()
$nexenRefStartupHash = "missing"
$nexenStartupHash = "missing"
$startupFirstDiff = -1
$nexenRefStartupCheckpointCount = 0
$nexenStartupCheckpointCount = 0
$nexenRefStartupDisplayTransitionCount = 0
$nexenStartupDisplayTransitionCount = 0
$nexenRefStartupPaletteCheckpointCount = 0
$nexenStartupPaletteCheckpointCount = 0
$nexenRefStartupVdpSnapshotCount = 0
$nexenStartupVdpSnapshotCount = 0
$nexenRefStartupVdpRegisterWriteCount = 0
$nexenStartupVdpRegisterWriteCount = 0
$nexenRefStartupTmssUnlockCount = 0
$nexenStartupTmssUnlockCount = 0
$nexenRefStartupZ80RuntimeToggleCount = 0
$nexenStartupZ80RuntimeToggleCount = 0
$nexenRefStartupZ80BusReqEventCount = 0
$nexenStartupZ80BusReqEventCount = 0
$nexenRefStartupZ80ResetEventCount = 0
$nexenStartupZ80ResetEventCount = 0
$nexenRefStartupNormalizedLines = @()
$nexenStartupNormalizedLines = @()
$startupNormalizedFirstDiff = -1
$nexenRefFrameSnapshot = [ordered]@{}
$nexenFrameSnapshot = [ordered]@{}

if ($null -ne $nexenRefTracePath) {
	$nexenRefLines = Get-TraceLines -Path $nexenRefTracePath
	$nexenRefHash = (Get-FileHash -LiteralPath $nexenRefTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($null -ne $nexenTracePath) {
	$nexenLines = Get-TraceLines -Path $nexenTracePath
	$nexenHash = (Get-FileHash -LiteralPath $nexenTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($null -ne $nexenRefStartupTracePath) {
	$nexenRefStartupLines = Get-TraceLines -Path $nexenRefStartupTracePath
	$nexenRefStartupHash = (Get-FileHash -LiteralPath $nexenRefStartupTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($null -ne $nexenStartupTracePath) {
	$nexenStartupLines = Get-TraceLines -Path $nexenStartupTracePath
	$nexenStartupHash = (Get-FileHash -LiteralPath $nexenStartupTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($nexenRefLines.Count -gt 0 -or $nexenLines.Count -gt 0) {
	$firstDiff = Find-FirstDifference -Left $nexenRefLines -Right $nexenLines
}

if ($nexenRefStartupLines.Count -gt 0 -or $nexenStartupLines.Count -gt 0) {
	$startupFirstDiff = Find-FirstDifference -Left $nexenRefStartupLines -Right $nexenStartupLines
	$nexenRefStartupNormalizedLines = Get-NormalizedStartupLines -Lines $nexenRefStartupLines
	$nexenStartupNormalizedLines = Get-NormalizedStartupLines -Lines $nexenStartupLines
	$startupNormalizedFirstDiff = Find-FirstDifference -Left $nexenRefStartupNormalizedLines -Right $nexenStartupNormalizedLines
	$nexenRefStartupMetrics = Get-StartupMetrics -Lines $nexenRefStartupLines
	$nexenStartupMetrics = Get-StartupMetrics -Lines $nexenStartupLines
	$nexenRefFrameSnapshot = Get-StartupFrameSnapshot -Lines $nexenRefStartupLines -Frame $SnapshotFrame
	$nexenFrameSnapshot = Get-StartupFrameSnapshot -Lines $nexenStartupLines -Frame $SnapshotFrame
	$nexenRefLoopPoll16Stats = Get-StartupFrameTagStats -Lines $nexenRefStartupLines -Frame $SnapshotFrame -Tag "LOOP_POLL16"
	$nexenLoopPoll16Stats = Get-StartupFrameTagStats -Lines $nexenStartupLines -Frame $SnapshotFrame -Tag "LOOP_POLL16"
	$nexenRefCpuPreloopStats = Get-StartupFrameTagStats -Lines $nexenRefStartupLines -Frame $SnapshotFrame -Tag "CPU_PRELOOP"
	$nexenCpuPreloopStats = Get-StartupFrameTagStats -Lines $nexenStartupLines -Frame $SnapshotFrame -Tag "CPU_PRELOOP"
	$nexenRefCpuPreloopGlobalStats = Get-StartupTagGlobalStats -Lines $nexenRefStartupLines -Tag "CPU_PRELOOP"
	$nexenCpuPreloopGlobalStats = Get-StartupTagGlobalStats -Lines $nexenStartupLines -Tag "CPU_PRELOOP"
	$nexenRefCpuLoopStats = Get-StartupFrameTagStats -Lines $nexenRefStartupLines -Frame $SnapshotFrame -Tag "CPU_LOOP"
	$nexenCpuLoopStats = Get-StartupFrameTagStats -Lines $nexenStartupLines -Frame $SnapshotFrame -Tag "CPU_LOOP"

	$nexenRefStartupCheckpointCount = $nexenRefStartupMetrics.startupCheckpointCount
	$nexenStartupCheckpointCount = $nexenStartupMetrics.startupCheckpointCount
	$nexenRefStartupDisplayTransitionCount = $nexenRefStartupMetrics.startupDisplayTransitionCount
	$nexenStartupDisplayTransitionCount = $nexenStartupMetrics.startupDisplayTransitionCount
	$nexenRefStartupPaletteCheckpointCount = $nexenRefStartupMetrics.startupPaletteCheckpointCount
	$nexenStartupPaletteCheckpointCount = $nexenStartupMetrics.startupPaletteCheckpointCount
	$nexenRefStartupVdpSnapshotCount = $nexenRefStartupMetrics.startupVdpSnapshotCount
	$nexenStartupVdpSnapshotCount = $nexenStartupMetrics.startupVdpSnapshotCount
	$nexenRefStartupVdpRegisterWriteCount = $nexenRefStartupMetrics.startupVdpRegisterWriteCount
	$nexenStartupVdpRegisterWriteCount = $nexenStartupMetrics.startupVdpRegisterWriteCount
	$nexenRefStartupTmssUnlockCount = $nexenRefStartupMetrics.startupTmssUnlockCount
	$nexenStartupTmssUnlockCount = $nexenStartupMetrics.startupTmssUnlockCount
	$nexenRefStartupZ80RuntimeToggleCount = $nexenRefStartupMetrics.startupZ80RuntimeToggleCount
	$nexenStartupZ80RuntimeToggleCount = $nexenStartupMetrics.startupZ80RuntimeToggleCount
	$nexenRefStartupZ80BusReqEventCount = $nexenRefStartupMetrics.startupZ80BusReqEventCount
	$nexenStartupZ80BusReqEventCount = $nexenStartupMetrics.startupZ80BusReqEventCount
	$nexenRefStartupZ80ResetEventCount = $nexenRefStartupMetrics.startupZ80ResetEventCount
	$nexenStartupZ80ResetEventCount = $nexenStartupMetrics.startupZ80ResetEventCount
}

$report = New-Object System.Collections.Generic.List[string]
$report.Add("GENESIS_CROSS_EMU_WRAM_TRACE_COMPARE")
$report.Add("rom=$resolvedRom")
$report.Add("frameStart=$FrameStart")
$report.Add("frameEnd=$FrameEnd")
$report.Add("autoStopTimeoutSeconds=$AutoStopTimeoutSeconds")
$report.Add("maxLines=$MaxLines")
$report.Add("nexenRefTrace=$nexenRefTracePath")
$report.Add("nexenTrace=$nexenTracePath")
$report.Add("nexenRefStartupTrace=$nexenRefStartupTracePath")
$report.Add("nexenStartupTrace=$nexenStartupTracePath")
$report.Add("nexenRefTraceSourceKind=$($nexenRefTraceResult.SourceKind)")
$report.Add("nexenTraceSourceKind=$($nexenTraceResult.SourceKind)")
$report.Add("nexenRefStartupTraceSourceKind=$($nexenRefStartupTraceResult.SourceKind)")
$report.Add("nexenStartupTraceSourceKind=$($nexenStartupTraceResult.SourceKind)")
$report.Add("nexenRefTraceSourcePath=$($nexenRefTraceResult.SourcePath)")
$report.Add("nexenTraceSourcePath=$($nexenTraceResult.SourcePath)")
$report.Add("nexenRefStartupTraceSourcePath=$($nexenRefStartupTraceResult.SourcePath)")
$report.Add("nexenStartupTraceSourcePath=$($nexenStartupTraceResult.SourcePath)")
$report.Add("nexenRefLines=$($nexenRefLines.Count)")
$report.Add("nexenLines=$($nexenLines.Count)")
$report.Add("nexenRefStartupLines=$($nexenRefStartupLines.Count)")
$report.Add("nexenStartupLines=$($nexenStartupLines.Count)")
$report.Add("snapshotFrame=$SnapshotFrame")
$report.Add("nexenRefStartupNormalizedLines=$($nexenRefStartupNormalizedLines.Count)")
$report.Add("nexenStartupNormalizedLines=$($nexenStartupNormalizedLines.Count)")
$report.Add("nexenRefStartupCheckpointCount=$nexenRefStartupCheckpointCount")
$report.Add("nexenStartupCheckpointCount=$nexenStartupCheckpointCount")
$report.Add("nexenRefStartupDisplayTransitionCount=$nexenRefStartupDisplayTransitionCount")
$report.Add("nexenStartupDisplayTransitionCount=$nexenStartupDisplayTransitionCount")
$report.Add("nexenRefStartupPaletteCheckpointCount=$nexenRefStartupPaletteCheckpointCount")
$report.Add("nexenStartupPaletteCheckpointCount=$nexenStartupPaletteCheckpointCount")
$report.Add("nexenRefStartupVdpSnapshotCount=$nexenRefStartupVdpSnapshotCount")
$report.Add("nexenStartupVdpSnapshotCount=$nexenStartupVdpSnapshotCount")
$report.Add("nexenRefStartupVdpRegisterWriteCount=$nexenRefStartupVdpRegisterWriteCount")
$report.Add("nexenStartupVdpRegisterWriteCount=$nexenStartupVdpRegisterWriteCount")
$report.Add("nexenRefStartupTmssUnlockCount=$nexenRefStartupTmssUnlockCount")
$report.Add("nexenStartupTmssUnlockCount=$nexenStartupTmssUnlockCount")
$report.Add("nexenRefStartupZ80RuntimeToggleCount=$nexenRefStartupZ80RuntimeToggleCount")
$report.Add("nexenStartupZ80RuntimeToggleCount=$nexenStartupZ80RuntimeToggleCount")
$report.Add("nexenRefStartupZ80BusReqEventCount=$nexenRefStartupZ80BusReqEventCount")
$report.Add("nexenStartupZ80BusReqEventCount=$nexenStartupZ80BusReqEventCount")
$report.Add("nexenRefStartupZ80ResetEventCount=$nexenRefStartupZ80ResetEventCount")
$report.Add("nexenStartupZ80ResetEventCount=$nexenStartupZ80ResetEventCount")
$report.Add("nexenRefHash=$nexenRefHash")
$report.Add("nexenHash=$nexenHash")
$report.Add("nexenRefStartupHash=$nexenRefStartupHash")
$report.Add("nexenStartupHash=$nexenStartupHash")
$report.Add("nexenRefFrontendMissing=$($null -eq $resolvedNexenRefExe)")
$report.Add("nexenRefRunSkipped=$nexenRefRunSkipped")
$report.Add("nexenRefTraceMissing=$($null -eq $nexenRefTracePath)")
$report.Add("nexenTraceMissing=$($null -eq $nexenTracePath)")
$report.Add("nexenRefStartupTraceMissing=$($null -eq $nexenRefStartupTracePath)")
$report.Add("nexenStartupTraceMissing=$($null -eq $nexenStartupTracePath)")
$report.Add("nexenRefTimedOut=$($nexenRefRun.StoppedByTimeout)")
$report.Add("nexenTimedOut=$($nexenRun.StoppedByTimeout)")
$report.Add("nexenRefExitCode=$($nexenRefRun.ExitCode)")
$report.Add("nexenExitCode=$($nexenRun.ExitCode)")
$report.Add("nexenRefLaunchTarget=$($nexenRefRun.LaunchTarget)")
$report.Add("nexenRefLaunchArgs=$($nexenRefRun.LaunchArgs)")
$report.Add("nexenLaunchTarget=$($nexenRun.LaunchTarget)")
$report.Add("nexenLaunchArgs=$($nexenRun.LaunchArgs)")
$report.Add("nexenRefStartupTraceEnvSupported=$nexenRefStartupTraceEnvSupported")
$report.Add("nexenRefSearchRoots=$($nexenRefSearchRoots -join ';')")
$report.Add("nexenSearchRoots=$($nexenSearchRoots -join ';')")
$report.Add("nexenRefTraceCandidates=$($nexenRefTraceCandidates -join ';')")
$report.Add("nexenRefStartupTraceCandidates=$($nexenRefStartupTraceCandidates -join ';')")
$report.Add("nexenTraceCandidates=$($nexenTraceCandidates -join ';')")
$report.Add("nexenStartupTraceCandidates=$($nexenStartupTraceCandidates -join ';')")
if ($nexenRefAttemptNotes.Count -gt 0) {
	$report.Add("nexenRefAttempts=$($nexenRefAttemptNotes -join ' | ')")
}
$report.Add("firstDiffIndex=$firstDiff")
$report.Add("startupFirstDiffIndex=$startupFirstDiff")
$report.Add("startupNormalizedFirstDiffIndex=$startupNormalizedFirstDiff")

if ($firstDiff -ge 0) {
	$nexenRefFirst = if ($firstDiff -lt $nexenRefLines.Count) { $nexenRefLines[$firstDiff] } else { "<no-line>" }
	$nexenFirst = if ($firstDiff -lt $nexenLines.Count) { $nexenLines[$firstDiff] } else { "<no-line>" }
	$report.Add("nexenRefFirstDiff=$nexenRefFirst")
	$report.Add("nexenFirstDiff=$nexenFirst")
} else {
	$report.Add("status=identical")
}

if ($startupFirstDiff -ge 0) {
	$nexenRefStartupFirst = if ($startupFirstDiff -lt $nexenRefStartupLines.Count) { $nexenRefStartupLines[$startupFirstDiff] } else { "<no-line>" }
	$nexenStartupFirst = if ($startupFirstDiff -lt $nexenStartupLines.Count) { $nexenStartupLines[$startupFirstDiff] } else { "<no-line>" }
	$report.Add("nexenRefStartupFirstDiff=$nexenRefStartupFirst")
	$report.Add("nexenStartupFirstDiff=$nexenStartupFirst")
}

if ($startupNormalizedFirstDiff -ge 0) {
	$nexenRefStartupNormalizedFirst = if ($startupNormalizedFirstDiff -lt $nexenRefStartupNormalizedLines.Count) { $nexenRefStartupNormalizedLines[$startupNormalizedFirstDiff] } else { "<no-line>" }
	$nexenStartupNormalizedFirst = if ($startupNormalizedFirstDiff -lt $nexenStartupNormalizedLines.Count) { $nexenStartupNormalizedLines[$startupNormalizedFirstDiff] } else { "<no-line>" }
	$report.Add("nexenRefStartupNormalizedFirstDiff=$nexenRefStartupNormalizedFirst")
	$report.Add("nexenStartupNormalizedFirstDiff=$nexenStartupNormalizedFirst")
}

$report.Add("nexenRefSnapshot_STARTUP_PAL=$($nexenRefFrameSnapshot.STARTUP_PAL)")
$report.Add("nexenSnapshot_STARTUP_PAL=$($nexenFrameSnapshot.STARTUP_PAL)")
$report.Add("nexenRefSnapshot_STARTUP_VDP=$($nexenRefFrameSnapshot.STARTUP_VDP)")
$report.Add("nexenSnapshot_STARTUP_VDP=$($nexenFrameSnapshot.STARTUP_VDP)")
$report.Add("nexenRefSnapshot_VDP_REG_W=$($nexenRefFrameSnapshot.VDP_REG_W)")
$report.Add("nexenSnapshot_VDP_REG_W=$($nexenFrameSnapshot.VDP_REG_W)")
$report.Add("nexenRefSnapshot_VDP_STAT_W=$($nexenRefFrameSnapshot.VDP_STAT_W)")
$report.Add("nexenSnapshot_VDP_STAT_W=$($nexenFrameSnapshot.VDP_STAT_W)")
$report.Add("nexenRefSnapshot_LOOP_POLL8=$($nexenRefFrameSnapshot.LOOP_POLL8)")
$report.Add("nexenSnapshot_LOOP_POLL8=$($nexenFrameSnapshot.LOOP_POLL8)")
$report.Add("nexenRefSnapshot_LOOP_POLL16=$($nexenRefFrameSnapshot.LOOP_POLL16)")
$report.Add("nexenSnapshot_LOOP_POLL16=$($nexenFrameSnapshot.LOOP_POLL16)")
$report.Add("nexenRefSnapshot_CPU_PRELOOP=$($nexenRefFrameSnapshot.CPU_PRELOOP)")
$report.Add("nexenSnapshot_CPU_PRELOOP=$($nexenFrameSnapshot.CPU_PRELOOP)")
$report.Add("nexenRefSnapshot_CPU_LOOP=$($nexenRefFrameSnapshot.CPU_LOOP)")
$report.Add("nexenSnapshot_CPU_LOOP=$($nexenFrameSnapshot.CPU_LOOP)")
$report.Add("nexenRefLoopPoll16Count=$($nexenRefLoopPoll16Stats.Count)")
$report.Add("nexenLoopPoll16Count=$($nexenLoopPoll16Stats.Count)")
$report.Add("nexenRefLoopPoll16First=$($nexenRefLoopPoll16Stats.First)")
$report.Add("nexenLoopPoll16First=$($nexenLoopPoll16Stats.First)")
$report.Add("nexenRefLoopPoll16Last=$($nexenRefLoopPoll16Stats.Last)")
$report.Add("nexenLoopPoll16Last=$($nexenLoopPoll16Stats.Last)")
$report.Add("nexenRefCpuPreloopCount=$($nexenRefCpuPreloopStats.Count)")
$report.Add("nexenCpuPreloopCount=$($nexenCpuPreloopStats.Count)")
$report.Add("nexenRefCpuPreloopFirst=$($nexenRefCpuPreloopStats.First)")
$report.Add("nexenCpuPreloopFirst=$($nexenCpuPreloopStats.First)")
$report.Add("nexenRefCpuPreloopLast=$($nexenRefCpuPreloopStats.Last)")
$report.Add("nexenCpuPreloopLast=$($nexenCpuPreloopStats.Last)")
$report.Add("nexenRefCpuPreloopGlobalCount=$($nexenRefCpuPreloopGlobalStats.Count)")
$report.Add("nexenCpuPreloopGlobalCount=$($nexenCpuPreloopGlobalStats.Count)")
$report.Add("nexenRefCpuPreloopGlobalFirstFrame=$($nexenRefCpuPreloopGlobalStats.FirstFrame)")
$report.Add("nexenCpuPreloopGlobalFirstFrame=$($nexenCpuPreloopGlobalStats.FirstFrame)")
$report.Add("nexenRefCpuPreloopGlobalLastFrame=$($nexenRefCpuPreloopGlobalStats.LastFrame)")
$report.Add("nexenCpuPreloopGlobalLastFrame=$($nexenCpuPreloopGlobalStats.LastFrame)")
$report.Add("nexenRefCpuPreloopGlobalFirst=$($nexenRefCpuPreloopGlobalStats.First)")
$report.Add("nexenCpuPreloopGlobalFirst=$($nexenCpuPreloopGlobalStats.First)")
$report.Add("nexenRefCpuPreloopGlobalLast=$($nexenRefCpuPreloopGlobalStats.Last)")
$report.Add("nexenCpuPreloopGlobalLast=$($nexenCpuPreloopGlobalStats.Last)")
$report.Add("nexenRefCpuLoopCount=$($nexenRefCpuLoopStats.Count)")
$report.Add("nexenCpuLoopCount=$($nexenCpuLoopStats.Count)")
$report.Add("nexenRefCpuLoopFirst=$($nexenRefCpuLoopStats.First)")
$report.Add("nexenCpuLoopFirst=$($nexenCpuLoopStats.First)")
$report.Add("nexenRefCpuLoopLast=$($nexenRefCpuLoopStats.Last)")
$report.Add("nexenCpuLoopLast=$($nexenCpuLoopStats.Last)")

$report | Out-File -LiteralPath $resolvedReportPath -Encoding utf8

Write-Host "Trace comparison report: $resolvedReportPath" -ForegroundColor Green
Write-Host "nexenRefLines=$($nexenRefLines.Count) nexenLines=$($nexenLines.Count) firstDiffIndex=$firstDiff" -ForegroundColor Green
if ($null -eq $nexenRefTracePath) {
	Write-Warning "NexenRef trace file was not emitted."
}
if ($null -eq $nexenTracePath) {
	Write-Warning "Nexen trace file was not emitted."
}
if ($null -eq $nexenRefStartupTracePath) {
	if (-not $nexenRefStartupTraceEnvSupported) {
		Write-Warning "NexenRef startup trace file was not emitted (startup trace env support not detected in source tree)."
	} else {
		Write-Warning "NexenRef startup trace file was not emitted."
	}
}
if ($null -eq $nexenStartupTracePath) {
	Write-Warning "Nexen startup trace file was not emitted."
}
if ($firstDiff -ge 0) {
	Write-Host "First difference found at index $firstDiff" -ForegroundColor Yellow
	exit 3
}

if ($nexenRefRunSkipped) {
	Write-Warning "NexenRef run was skipped because no frontend executable was found."
	exit 0
}

exit 0




