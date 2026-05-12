# kissfft Package Migration Track

Issue: #2190
Parent: #2185

## Goal

Remove vendored `Utilities/kissfft.h` and consume kissfft from package-managed dependency.

## Constraints

- Preserve FFT behavior used by audio HUD/equalizer paths.
- Keep C++ build compatibility across Release/Debug/PGO configurations.
- Do not introduce additional vendored third-party source.

## Implemented Changes

1. Added `kissfft` package dependency in `vcpkg.json`.
2. Updated include in `Core/Shared/Audio/AudioPlayerHud.h`:
	- from `"Utilities/kissfft.h"`
	- to `<kissfft/kissfft.hh>`
3. Removed vendored file:
	- `Utilities/kissfft.h`
4. Removed vendored project/filter entries from:
	- `Utilities/Utilities.vcxproj`
	- `Utilities/Utilities.vcxproj.filters`
5. Updated third-party policy and inventory to remove kissfft vendored exception.

## Validation

- Packaged-mode benchmark target build passes:
	- `Core.Benchmarks/Core.Benchmarks.vcxproj`
	- `/p:NexenUsePackagedLua=true /p:NexenUsePackagedSpng=true`
- Third-party policy enforcement passes after migration:
	- `./scripts/audit-third-party-source.ps1 -Enforce`

## Status

Completed for vendored-source removal and package-backed integration.
