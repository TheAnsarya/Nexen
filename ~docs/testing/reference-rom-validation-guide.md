# Reference ROM Validation Manifest Guide

> How to use, populate, and extend the reference ROM validation manifest for Nexen regression testing.

## Overview

The **reference ROM validation manifest** is a curated JSON file that catalogues ROMs used for deterministic regression testing. Each entry describes a ROM by its checksum, expected mapper/controller type, target test harness, and a "summary digest" captured from a known-good emulation run.

The manifest **never** contains ROM data — only checksums and metadata. ROMs must be supplied locally at `C:\~reference-roms\`.

## Files

| File | Purpose |
|------|---------|
| [reference-rom-validation-manifest.json](reference-rom-validation-manifest.json) | Active manifest with ROM entries |
| [reference-rom-validation-manifest.schema.json](reference-rom-validation-manifest.schema.json) | JSON Schema 2020-12 formal definition |
| [reference-rom-validation-manifest.example.json](reference-rom-validation-manifest.example.json) | Original example/template (for reference only) |

## Manifest Structure

```jsonc
{
	"$schema": "reference-rom-validation-manifest.schema.json",
	"manifestVersion": "1.0",
	"description": "...",
	"systems": [
		{
			"platform": "atari2600",   // Platform identifier
			"entries": [
				{
					"id": "a26-2k-basic",              // Unique slug
					"romFileHint": "combat.a26",       // Expected filename in reference-roms dir
					"sourceNote": "...",               // Provenance/purpose note
					"sha256": "abc123...",             // SHA-256 of the canonical ROM
					"crc32": "0xdeadbeef",             // Optional CRC32
					"romSizeBytes": 2048,              // Optional exact size
					"expected": {
						"mapper": "2k",                // Expected mapper type
						"harness": "...Tests",         // GTest suite to run
						"summaryDigest": "...",        // Hash of frame output (see below)
						"controllerType": "joystick",  // Expected controller
						"audioChannels": 2,            // Optional: audio channel count
						"frameCount": 600              // Optional: frames to run
					},
					"tags": ["mapper-basic", "smoke-test"],
					"disabled": false                  // Set true to skip entry
				}
			]
		}
	]
}
```

## Field Reference

### Top Level

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `$schema` | string | no | Relative path to JSON Schema file |
| `manifestVersion` | string | yes | Schema version (currently `"1.0"`) |
| `description` | string | no | Human-readable description |
| `systems` | array | yes | Array of per-platform sections |

### System Section

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `platform` | string | yes | Platform ID: `atari2600`, `genesis`, `nes`, `snes`, `gb`, `gba`, `sms`, `pce`, `ws`, `lynx` |
| `entries` | array | yes | Array of ROM entries |

### ROM Entry

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | yes | Unique slug (e.g., `a26-f8-bankswitching`) |
| `romFileHint` | string | no | Expected filename in `C:\~reference-roms\{platform}\` |
| `sourceNote` | string | no | Provenance: where the ROM came from, why it was chosen |
| `sha256` | string | yes | SHA-256 hash of the canonical ROM file |
| `crc32` | string | no | CRC32 hash (hex with `0x` prefix) |
| `romSizeBytes` | integer | no | Exact ROM file size in bytes |
| `expected` | object | yes | Expected emulation results |
| `tags` | array | no | Classification tags for filtering |
| `disabled` | boolean | no | Set `true` to skip this entry during validation |

### Expected Results

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `mapper` | string | no | Expected mapper type (e.g., `2k`, `4k`, `f8`, `f6`, `e0`, `3f`, `fe`) |
| `titleClass` | string | no | Game genre/class (e.g., `platformer`, `rpg`, `action`) |
| `harness` | string | yes | GTest suite name to bind this ROM to |
| `summaryDigest` | string | yes | Hash of known-good frame output for regression detection |
| `frameCount` | integer | no | Number of frames to run for deterministic comparison |
| `controllerType` | string | no | Expected controller type (e.g., `joystick`, `paddle`, `keypad`, `driving`, `booster-grip`, `3button`, `6button`) |
| `audioChannels` | integer | no | Expected audio channel count |

## Populating the Manifest

### Step 1: Prepare Reference ROMs

Place your reference ROM files in the local directory:

```
C:\~reference-roms\
├── atari2600\
│   ├── combat.a26
│   ├── adventure.a26
│   ├── asteroids.a26
│   └── ...
├── genesis\
│   ├── sonic-the-hedgehog.md
│   └── ...
└── ...
```

### Step 2: Compute SHA-256 Hashes

```powershell
# Compute SHA-256 for a single ROM
(Get-FileHash "C:\~reference-roms\atari2600\combat.a26" -Algorithm SHA256).Hash.ToLower()

# Compute for all ROMs in a directory
Get-ChildItem "C:\~reference-roms\atari2600\*" | ForEach-Object {
	$hash = (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower()
	"$($_.Name): $hash"
}
```

### Step 3: Update Manifest Entries

Replace placeholder `"replace-with-real-sha256"` values with the computed hashes.

### Step 4: Capture Known-Good Summary Digests

Run each ROM through Nexen for a fixed number of frames and capture a deterministic hash of the output:

```powershell
# Run a ROM through the test harness to capture a digest
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600TimingSpikeHarnessTests.* --gtest_brief=1
```

Update the `summaryDigest` field with the captured values.

### Step 5: Validate

```powershell
# Re-run the harness to confirm deterministic output
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600*Tests.* --gtest_brief=1
```

## Coverage Strategy

### Atari 2600 Coverage Goals

The manifest aims to cover:

- **Mappers:** 2K, 4K, F8, F6, F4, E0, 3F, FE (all mapper types supported by Nexen)
- **Controllers:** Joystick, Paddle, Keypad, Driving, Booster-Grip (all implemented controller types)
- **Rendering:** Timing-sensitive kernels, collision-heavy games, sprite-heavy scenes
- **Audio:** TIA audio channels, DPC audio
- **Performance:** Regression benchmark ROMs with fixed frame counts

### Genesis Coverage Goals

- **VDP:** Scrolling, sprite layers, mode switching, H-interrupt timing
- **Audio:** YM2612 FM synthesis, PSG
- **Bus:** Z80 bus arbitration
- **Controllers:** 3-button and 6-button gamepads

## Integration Points

### 1. GTest Harnesses

The `harness` field in each entry maps to a C++ GTest suite. Test harnesses should:

- Accept a ROM path (resolved from `romFileHint` via `C:\~reference-roms\{platform}\`)
- Verify the ROM's SHA-256 matches the manifest before testing
- Run a fixed number of frames (from `frameCount` or a default)
- Compute a summary digest of the output (frame buffer hash, audio hash, or combined)
- Compare against the manifest's `summaryDigest` value

### 2. CI Pipeline

A future CI script can:

1. Read the manifest JSON
2. For each non-disabled entry, locate the ROM by SHA-256
3. Run the bound GTest harness
4. Report pass/fail per entry
5. Flag any missing ROMs (skip gracefully)

### 3. Tag-Based Filtering

Tags enable selective test runs:

```powershell
# Run only smoke-test entries
# (CI script filters manifest by tag before invoking harnesses)
```

### 4. Disabled Entries

Set `"disabled": true` on entries whose ROMs are not yet available or whose harnesses are not yet implemented. This prevents CI failures while keeping the entry in the manifest for future activation.

## Adding New Entries

1. Choose a unique `id` following the convention: `{platform-prefix}-{category}-{descriptor}`
	- Atari 2600: `a26-{mapper|controller|feature}-{name}`
	- Genesis: `gen-{feature}-{name}`
2. Add the ROM to `C:\~reference-roms\{platform}\`
3. Compute the SHA-256 hash
4. Add the entry to the appropriate platform section in the manifest
5. Run the harness once to capture the `summaryDigest`
6. Update the manifest with the digest
7. Commit the manifest change

## Related Documentation

- [Testing Documentation Index](README.md)
- [Atari 2600 Test Plan](atari-2600-test-plan.md)
- [Atari 2600 Testing Status](atari-2600-testing-status.md)
