# Accuracy Testing Pipeline

> Step-by-step reproducible commands for downloading, organizing, and running accuracy test ROMs in Nexen.

## Prerequisites

- Nexen built in Release x64 configuration
- PowerShell 7+
- Git (for cloning test ROM repos)
- Python 3.10+ (optional, for helper scripts)
- Internet access (for initial downloads)

---

## Step 1: Create Folder Structure

```powershell
# Create the reference-roms directory hierarchy
$base = "c:\reference-roms"
$systems = @("nes", "snes", "gb", "gba", "genesis", "sms", "pce", "ws", "lynx")

foreach ($sys in $systems) {
	New-Item -ItemType Directory -Force -Path "$base\$sys\community"
	New-Item -ItemType Directory -Force -Path "$base\$sys\custom"
}

# NES subdirectories
New-Item -Force -ItemType Directory -Path "$base\nes\blargg"
New-Item -Force -ItemType Directory -Path "$base\nes\accuracycoin"
New-Item -Force -ItemType Directory -Path "$base\nes\kevtris"
New-Item -Force -ItemType Directory -Path "$base\nes\bisqwit"
New-Item -Force -ItemType Directory -Path "$base\nes\tepples"
New-Item -Force -ItemType Directory -Path "$base\nes\mapper-tests"
New-Item -Force -ItemType Directory -Path "$base\nes\misc"

# SNES subdirectories
New-Item -Force -ItemType Directory -Path "$base\snes\blargg"
New-Item -Force -ItemType Directory -Path "$base\snes\official-aging"
New-Item -Force -ItemType Directory -Path "$base\snes\undisbeliever"
New-Item -Force -ItemType Directory -Path "$base\snes\peterlemon"
New-Item -Force -ItemType Directory -Path "$base\snes\coprocessor"
New-Item -Force -ItemType Directory -Path "$base\snes\hirom-gsu-test"
New-Item -Force -ItemType Directory -Path "$base\snes\misc"

# GB subdirectories
New-Item -Force -ItemType Directory -Path "$base\gb\mooneye"
New-Item -Force -ItemType Directory -Path "$base\gb\blargg"
New-Item -Force -ItemType Directory -Path "$base\gb\acid2"
New-Item -Force -ItemType Directory -Path "$base\gb\samesuite"

# GBA subdirectories
New-Item -Force -ItemType Directory -Path "$base\gba\jsmolka"
New-Item -Force -ItemType Directory -Path "$base\gba\mgba-suite"
New-Item -Force -ItemType Directory -Path "$base\gba\fuzzarm"
New-Item -Force -ItemType Directory -Path "$base\gba\armwrestler"

# Genesis subdirectories
New-Item -Force -ItemType Directory -Path "$base\genesis\blastem"
New-Item -Force -ItemType Directory -Path "$base\genesis\peterlemon"
New-Item -Force -ItemType Directory -Path "$base\genesis\zexall"
New-Item -Force -ItemType Directory -Path "$base\genesis\vdp-tests"

# SMS subdirectories
New-Item -Force -ItemType Directory -Path "$base\sms\flubba"
New-Item -Force -ItemType Directory -Path "$base\sms\zexall"
New-Item -Force -ItemType Directory -Path "$base\sms\vdp-tests"
```

## Step 2: Download Test ROMs

### NES

```powershell
# AccuracyCoin (includes prebuilt .nes)
git clone https://github.com/100thCoin/AccuracyCoin.git "$base\nes\accuracycoin\repo"
Copy-Item "$base\nes\accuracycoin\repo\AccuracyCoin.nes" "$base\nes\accuracycoin\"

# nes-test-roms archive (mirrors many blargg tests)
git clone https://github.com/christopherpow/nes-test-roms.git "$base\nes\nes-test-roms-archive"

# Copy blargg tests from archive
Copy-Item -Recurse "$base\nes\nes-test-roms-archive\blargg*" "$base\nes\blargg\"

# nestest
# Download from: https://www.nesdev.org/wiki/Emulator_tests (or archive)
# Place at: $base\nes\kevtris\nestest.nes
```

### SNES

```powershell
# PeterLemon SNES tests (includes prebuilt .sfc files)
git clone https://github.com/PeterLemon/SNES.git "$base\snes\peterlemon\repo"

# undisbeliever test ROMs
git clone https://github.com/undisbeliever/snes-test-roms.git "$base\snes\undisbeliever\repo"

# SNES-HiRomGsuTest
git clone https://github.com/astrobleem/SNES-HiRomGsuTest.git "$base\snes\hirom-gsu-test\repo"

# gilyon SNES tests
git clone https://github.com/gilyon/snes-tests.git "$base\snes\misc\gilyon"
```

### Game Boy

```powershell
# Mooneye Test Suite (prebuilt ROMs available)
git clone https://github.com/Gekkio/mooneye-test-suite.git "$base\gb\mooneye\repo"

# Download prebuilt Mooneye ROMs
# Available at: https://gekkio.fi/files/mooneye-test-suite/
# Place in: $base\gb\mooneye\prebuilt\

# dmg-acid2 and cgb-acid2
git clone https://github.com/mattcurrie/dmg-acid2.git "$base\gb\acid2\dmg"
git clone https://github.com/mattcurrie/cgb-acid2.git "$base\gb\acid2\cgb"

# SameSuite
git clone https://github.com/LIJI32/SameSuite.git "$base\gb\samesuite\repo"
```

### GBA

```powershell
# jsmolka/gba-tests
git clone https://github.com/jsmolka/gba-tests.git "$base\gba\jsmolka\repo"

# mGBA test suite
git clone https://github.com/mgba-emu/suite.git "$base\gba\mgba-suite\repo"
```

### Genesis

```powershell
# PeterLemon MD tests
git clone https://github.com/PeterLemon/MD.git "$base\genesis\peterlemon\repo"
```

## Step 3: Run a Single Test (Manual Verification)

### NES — AccuracyCoin

1. Launch Nexen with the test ROM:

```powershell
cd c:\Users\me\source\repos\Nexen
.\bin\win-x64\Release\Nexen.exe "c:\reference-roms\nes\accuracycoin\AccuracyCoin.nes"
```

2. Navigate the menu (press Start to run all tests on current page)
3. Check results table for PASS/FAIL

### NES — Blargg cpu_instrs (Headless)

```powershell
.\bin\win-x64\Release\Nexen.exe "c:\reference-roms\nes\blargg\cpu_instrs\cpu_instrs.nes" --novideo --noaudio --noinput --timeout=60
```

### GB — Mooneye (with Lua verification)

```powershell
.\bin\win-x64\Release\Nexen.exe "c:\reference-roms\gb\mooneye\prebuilt\acceptance\timer.gb" "~scripts\gb\mooneye-check.lua" --novideo --noaudio --timeout=20
```

## Step 4: Batch Test Execution

### PowerShell Batch Runner

```powershell
# run-accuracy-tests.ps1
param(
	[string]$NexenPath = ".\bin\win-x64\Release\Nexen.exe",
	[string]$TestRomBase = "c:\reference-roms",
	[string]$ScriptBase = ".\~scripts\accuracy",
	[int]$TimeoutSeconds = 60,
	[string]$ResultsPath = ".\~docs\testing\accuracy-results.json"
)

$results = @()
$passed = 0
$failed = 0

# Run each test ROM with its corresponding Lua script
$manifest = Get-Content ".\tests\accuracy\manifest.json" | ConvertFrom-Json

foreach ($test in $manifest.tests) {
	$romPath = Join-Path $TestRomBase $test.rom
	if (-not (Test-Path $romPath)) {
		Write-Warning "ROM not found: $romPath"
		$results += @{ name = $test.name; status = "SKIP"; reason = "ROM not found" }
		continue
	}

	$args = @($romPath, "--novideo", "--noaudio", "--noinput", "--timeout=$TimeoutSeconds")
	if ($test.script) {
		$scriptPath = Join-Path $ScriptBase $test.script
		$args += $scriptPath
	}

	$proc = Start-Process -FilePath $NexenPath -ArgumentList $args -Wait -PassThru -NoNewWindow
	$exitCode = $proc.ExitCode

	if ($exitCode -eq 0) {
		$passed++
		$results += @{ name = $test.name; status = "PASS"; exitCode = $exitCode }
	} else {
		$failed++
		$results += @{ name = $test.name; status = "FAIL"; exitCode = $exitCode }
	}

	Write-Host "$($test.name): $(if ($exitCode -eq 0) { 'PASS' } else { 'FAIL' }) (exit=$exitCode)"
}

Write-Host "`n=== Results: $passed passed, $failed failed, $($results.Count) total ==="

$results | ConvertTo-Json -Depth 3 | Set-Content $ResultsPath
```

### Running the Batch

```powershell
# From Nexen repo root
.\~scripts\accuracy\run-accuracy-tests.ps1
```

## Step 5: Record a Baseline (Golden Reference)

For visual regression testing using RecordedRomTest:

```powershell
# 1. Record golden reference for a test ROM
# This requires running Nexen with GUI, playing through the test, and recording frame hashes
# The recording is saved as a .mrt file

# 2. Future runs compare against the golden reference
.\bin\win-x64\Release\Nexen.exe --run-recorded-test "tests\accuracy\golden\cpu_instrs.mrt"
```

## Step 6: Review Results

Results are written to `~docs/testing/accuracy-results.json` and can be viewed:

```powershell
# Quick summary
$results = Get-Content ".\~docs\testing\accuracy-results.json" | ConvertFrom-Json
$results | Group-Object status | Select-Object Name, Count | Format-Table
```

---

## Automation Checklist

- [ ] Create `c:\reference-roms\` folder structure
- [ ] Download NES test ROMs (AccuracyCoin, blargg, nestest)
- [ ] Download SNES test ROMs (PeterLemon, undisbeliever, HiRomGsuTest)
- [ ] Download GB test ROMs (Mooneye, blargg, acid2)
- [ ] Download GBA test ROMs (jsmolka, mGBA suite)
- [ ] Create test manifest JSON
- [ ] Write Lua test scripts for each test type
- [ ] Write batch runner PowerShell script
- [ ] Run initial baseline on current Nexen build
- [ ] Record golden reference frame hashes
- [ ] Integrate into CI workflow

---

## Related Documents

- [Accuracy Test ROM Catalog](accuracy-test-rom-catalog.md) — Complete test ROM listing
- [Automated Accuracy Testing Plan](automated-accuracy-testing-plan.md) — Architecture design
- [In-House Test ROM Development Plan](in-house-test-rom-plan.md) — Custom test ROM creation
