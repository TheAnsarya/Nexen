# libspng Package Prototype Track

Issue: #2188
Parent: #2185

## Goal

Prepare an opt-in package-backed `libspng` path without changing default runtime behavior.

## Constraints

- Default build must remain vendored-source compatible.
- No behavior regressions in PNG encode/decode paths.
- Keep third-party source inventory from growing.

## Current State

- `Utilities/spng.c` is compiled directly in `Utilities/Utilities.vcxproj`.
- `Utilities/PNGHelper.cpp` includes local `spng.h`.
- Inventory baseline for libspng:
	- Files: 2
	- Lines: 4213
	- Bytes: 157549

## Prototype Changes

1. Add `NexenUsePackagedSpng` MSBuild property (default `false`).
2. Add compile define `NEXEN_USE_PACKAGED_SPNG` in `Utilities/Utilities.vcxproj`.
3. Condition vendored `spng.c` compile item on `NexenUsePackagedSpng != true`.
4. Add include abstraction in `Utilities/PNGHelper.cpp`:
	- packaged mode: `#include <spng.h>`
	- default mode: `#include "spng.h"`

## Validation

- Default Release x64 build remains unchanged.
- PNG helper compile path is valid in default mode.
- Follow-up (next batch): enable packaged mode with concrete dependency wiring and runtime checks.

## Exit Criteria

- Packaged mode links successfully with package-provided libspng.
- PNG load/save behavior validated by targeted tests/manual checks.
- Vendored libspng source removed and inventory/policy caps updated.
