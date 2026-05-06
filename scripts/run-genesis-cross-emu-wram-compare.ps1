param(
	[string]$RomPath,
	[string]$NexenExePath = ".\bin\win-x64\Release\Nexen.exe",
	[string]$MesenExePath = "..\Mesen2-Expanded\bin\win-x64\Release\Mesen.exe",
	[string]$NexenWorkingDir = ".",
	[string]$MesenWorkingDir = "..\Mesen2-Expanded",
	[string[]]$MesenArgs = @("--testRunner", "--timeout=35"),
	[string[]]$NexenArgs = @(),
	[switch]$AllowMissingMesenFrontend,
	[int]$AutoStopTimeoutSeconds = 30,
	[int]$FrameStart = 0,
	[int]$FrameEnd = 80,
	[string]$AddressStart = "0xE00000",
	[string]$AddressEnd = "0xFFFFFF",
	[int]$MaxLines = 300000,
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
	$launchArgs += $Rom
	if ([System.IO.Path]::GetExtension($ProgramPath).Equals('.dll', [System.StringComparison]::OrdinalIgnoreCase)) {
		$launchTarget = "dotnet"
		$launchArgs = @($ProgramPath)
		if ($PreArgs -and $PreArgs.Count -gt 0) {
			$launchArgs += $PreArgs
		}
		$launchArgs += $Rom
	}

	$process = Start-Process -FilePath $launchTarget -WorkingDirectory $WorkingDir -ArgumentList $launchArgs -PassThru
	if ($null -eq $process) {
		throw "Failed to launch $Name process."
	}

	Write-Host "$Name started: pid=$($process.Id)" -ForegroundColor Green
	$stoppedByTimeout = $false
	try {
		Wait-Process -Id $process.Id -Timeout $TimeoutSeconds -ErrorAction Stop
		Write-Host "$Name exited before timeout." -ForegroundColor Yellow
	} catch {
		Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
		$stoppedByTimeout = $true
		Write-Host "$Name auto-stopped after $TimeoutSeconds seconds." -ForegroundColor Yellow
	}

	return [pscustomobject]@{
		Pid = $process.Id
		StoppedByTimeout = $stoppedByTimeout
	}
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

$romCandidates = @()
if (-not [string]::IsNullOrWhiteSpace($RomPath)) {
	$romCandidates += $RomPath
}
$romCandidates += @(
	"C:\~reference-roms\genesis\Sonic The Hedgehog (W) (REV00) [!].bin",
	"C:\~reference-roms\genesis\Sonic The Hedgehog (W) (REV01) [!].bin",
	"C:\~reference-roms\genesis\Sonic The Hedgehog 2 (W) (REV01) [!].bin",
	"C:\~reference-roms\genesis\Sonic The Hedgehog (USA, Europe).md"
)

$mesenExeCandidates = @(
	$MesenExePath,
	"..\Mesen2-Expanded\bin\win-x64\Release\Mesen.exe",
	"..\Mesen2-Expanded\bin\win-x64\Debug\Mesen.exe"
)

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
$nexenTraceCandidates = @(
	(Join-Path $resolvedNexenDir "reference\cpu_ram_trace.log"),
	(Join-Path $nexenExeDir "reference\cpu_ram_trace.log")
)
$mesenTraceCandidates = @((Join-Path $resolvedMesenDir "reference\cpu_ram_trace.log"))
if ($null -ne $mesenExeDir) {
	$mesenTraceCandidates += (Join-Path $mesenExeDir "reference\cpu_ram_trace.log")
}
$mesenTraceCandidates += (Join-Path $mesenDocumentsDir "reference\cpu_ram_trace.log")
$resolvedReportPath = [System.IO.Path]::GetFullPath((Join-Path $resolvedNexenDir $ReportPath))
$reportDir = Split-Path -Parent $resolvedReportPath
if (!(Test-Path -LiteralPath $reportDir)) {
	New-Item -Path $reportDir -ItemType Directory -Force | Out-Null
}

foreach ($tracePath in ($nexenTraceCandidates + $mesenTraceCandidates)) {
	if (Test-Path -LiteralPath $tracePath) {
		Remove-Item -LiteralPath $tracePath -Force
	}
}

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
$mesenRunSkipped = $false

try {
	$env:MESEN_WRAM_FRAME_START = "$FrameStart"
	$env:MESEN_WRAM_FRAME_END = "$FrameEnd"
	$env:MESEN_WRAM_ADDR_START = "$AddressStart"
	$env:MESEN_WRAM_ADDR_END = "$AddressEnd"
	$env:MESEN_WRAM_MAX_LINES = "$MaxLines"

	$env:NEXEN_WRAM_FRAME_START = "$FrameStart"
	$env:NEXEN_WRAM_FRAME_END = "$FrameEnd"
	$env:NEXEN_WRAM_ADDR_START = "$AddressStart"
	$env:NEXEN_WRAM_ADDR_END = "$AddressEnd"
	$env:NEXEN_WRAM_MAX_LINES = "$MaxLines"

	if ($null -ne $resolvedMesenExe) {
		Stop-ExistingProcessInstances -ProgramPath $resolvedMesenExe -Name "Mesen2-Expanded"
	}
	Stop-ExistingProcessInstances -ProgramPath $resolvedNexenExe -Name "Nexen"

	$mesenRun = [pscustomobject]@{
		Pid = 0
		StoppedByTimeout = $false
	}
	if ($null -ne $resolvedMesenExe) {
		Write-Host "Running Mesen2-Expanded trace capture..." -ForegroundColor Cyan
		$mesenRun = Start-TimedRun -ProgramPath $resolvedMesenExe -PreArgs $MesenArgs -Rom $resolvedRom -WorkingDir $resolvedMesenDir -TimeoutSeconds $AutoStopTimeoutSeconds -Name "Mesen2-Expanded"
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
}

$mesenTracePath = TryResolveExistingTracePath -CandidatePaths $mesenTraceCandidates
$nexenTracePath = TryResolveExistingTracePath -CandidatePaths $nexenTraceCandidates

$mesenLines = @()
$nexenLines = @()
$firstDiff = -1
$mesenHash = "missing"
$nexenHash = "missing"

if ($null -ne $mesenTracePath) {
	$mesenLines = Get-TraceLines -Path $mesenTracePath
	$mesenHash = (Get-FileHash -LiteralPath $mesenTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($null -ne $nexenTracePath) {
	$nexenLines = Get-TraceLines -Path $nexenTracePath
	$nexenHash = (Get-FileHash -LiteralPath $nexenTracePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

if ($mesenLines.Count -gt 0 -or $nexenLines.Count -gt 0) {
	$firstDiff = Find-FirstDifference -Left $mesenLines -Right $nexenLines
}

$report = New-Object System.Collections.Generic.List[string]
$report.Add("GENESIS_CROSS_EMU_WRAM_TRACE_COMPARE")
$report.Add("rom=$resolvedRom")
$report.Add("mesenTrace=$mesenTracePath")
$report.Add("nexenTrace=$nexenTracePath")
$report.Add("mesenLines=$($mesenLines.Count)")
$report.Add("nexenLines=$($nexenLines.Count)")
$report.Add("mesenHash=$mesenHash")
$report.Add("nexenHash=$nexenHash")
$report.Add("mesenFrontendMissing=$($null -eq $resolvedMesenExe)")
$report.Add("mesenRunSkipped=$mesenRunSkipped")
$report.Add("mesenTraceMissing=$($null -eq $mesenTracePath)")
$report.Add("nexenTraceMissing=$($null -eq $nexenTracePath)")
$report.Add("mesenTimedOut=$($mesenRun.StoppedByTimeout)")
$report.Add("nexenTimedOut=$($nexenRun.StoppedByTimeout)")
$report.Add("firstDiffIndex=$firstDiff")

if ($firstDiff -ge 0) {
	$mesenFirst = if ($firstDiff -lt $mesenLines.Count) { $mesenLines[$firstDiff] } else { "<no-line>" }
	$nexenFirst = if ($firstDiff -lt $nexenLines.Count) { $nexenLines[$firstDiff] } else { "<no-line>" }
	$report.Add("mesenFirstDiff=$mesenFirst")
	$report.Add("nexenFirstDiff=$nexenFirst")
} else {
	$report.Add("status=identical")
}

$report | Out-File -LiteralPath $resolvedReportPath -Encoding utf8

Write-Host "Trace comparison report: $resolvedReportPath" -ForegroundColor Green
Write-Host "mesenLines=$($mesenLines.Count) nexenLines=$($nexenLines.Count) firstDiffIndex=$firstDiff" -ForegroundColor Green
if ($null -eq $mesenTracePath) {
	Write-Warning "Mesen2-Expanded trace file was not emitted."
}
if ($null -eq $nexenTracePath) {
	Write-Warning "Nexen trace file was not emitted."
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
