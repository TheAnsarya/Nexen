param(
	[string]$NexenExe = "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\Nexen.exe",
	[string]$RomRoot = "C:\~reference-roms\extracted\GoodLynx-v2.01\Selected1105",
	[string]$LuaScript = "c:\Users\me\source\repos\Nexen\~scripts\lynx\boot-smoke.lua",
	[string]$OutputJson = "c:\Users\me\source\repos\Nexen\~docs\research\lynx-commercial-bank-addressing-boot-smoke-results-2026-03-30.json",
	[int]$TimeoutSeconds = 45,
	[string]$TitleFilter = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (!(Test-Path -LiteralPath $NexenExe)) {
	throw "Nexen executable not found: $NexenExe"
}
if (!(Test-Path -LiteralPath $LuaScript)) {
	throw "Lua script not found: $LuaScript"
}
if (!(Test-Path -LiteralPath $RomRoot)) {
	throw "ROM root not found: $RomRoot"
}

$romSet = @(
	@{ Title = "California Games"; BaseName = "California Games (1991)" },
	@{ Title = "Chip's Challenge"; BaseName = "Chip's Challenge (1989)" },
	@{ Title = "Klax"; BaseName = "Klax (1990)" },
	@{ Title = "Awesome Golf"; BaseName = "Awesome Golf (1991)" },
	@{ Title = "Warbirds"; BaseName = "Warbirds (1990)" },
	@{ Title = "Electrocop"; BaseName = "Electrocop (1989)" },
	@{ Title = "Zarlor Mercenary"; BaseName = "Zarlor Mercenary (1990)" },
	@{ Title = "Blue Lightning"; BaseName = "Blue Lightning (1989)" },
	@{ Title = "Ninja Gaiden"; BaseName = "Ninja Gaiden (1990)" },
	@{ Title = "Gauntlet: The Third Encounter"; BaseName = "Gauntlet - The Third Encounter (1990)" }
)

$allRomFiles = Get-ChildItem -Path $RomRoot -Recurse -File -Filter "*.lnx"

$results = New-Object System.Collections.Generic.List[object]

foreach ($entry in $romSet) {
	if ($TitleFilter -ne "" -and $entry.Title -notlike "*$TitleFilter*") {
		continue
	}

	$romMatch = $allRomFiles | Where-Object { $_.BaseName -eq $entry.BaseName } | Select-Object -First 1
	if ($null -eq $romMatch) {
		$results.Add([pscustomobject]@{
			Title = $entry.Title
			RomFile = "$($entry.BaseName).lnx"
			RomPath = ""
			ExitCode = -404
			BootResult = "Missing"
			TimestampUtc = [DateTime]::UtcNow.ToString("o")
			Notes = "Canonical ROM file missing"
		})
		continue
	}

	$romPath = $romMatch.FullName
	$romFile = $romMatch.Name

	Write-Host "Running Lynx boot smoke: $($entry.Title)"
	$process = Start-Process -FilePath $NexenExe `
		-ArgumentList @("--testrunner", "--lynx.usebootrom=false", "--timeout=$TimeoutSeconds", $LuaScript, $romPath) `
		-Wait -PassThru -NoNewWindow
	$exitCode = [int]$process.ExitCode

	$bootResult = if ($exitCode -eq 0) { "Pass" } else { "Fail" }
	$results.Add([pscustomobject]@{
		Title = $entry.Title
		RomFile = $romFile
		RomPath = $romPath
		ExitCode = $exitCode
		BootResult = $bootResult
		TimestampUtc = [DateTime]::UtcNow.ToString("o")
		Notes = "Headless testrunner + Lua boot smoke"
	})
}

$outputDir = Split-Path -Parent $OutputJson
if (!(Test-Path -LiteralPath $outputDir)) {
	New-Item -ItemType Directory -Path $outputDir | Out-Null
}

$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $OutputJson -Encoding utf8
Write-Host "Saved results to: $OutputJson"
