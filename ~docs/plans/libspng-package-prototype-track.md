# libspng Package Prototype Track

Issue: #2188
Parent: #2185

## Goal

Complete migration from vendored `libspng` source to package-managed dependency usage.

## Constraints

- Default build must remain vendored-source compatible.
- No behavior regressions in PNG encode/decode paths.
- Keep third-party source inventory from growing.

## Current State

- Vendored `Utilities/spng.c` and `Utilities/spng.h` were removed.
- `Utilities/PNGHelper.cpp` now uses package include `<spng.h>`.
- `Utilities/Utilities.vcxproj` defaults to packaged mode (`NexenUsePackagedSpng=true`).

## Implemented Changes

1. Add `NexenUsePackagedSpng` MSBuild property (default `true`).
2. Add compile define `NEXEN_USE_PACKAGED_SPNG` in `Utilities/Utilities.vcxproj`.
3. Remove vendored `spng.c` compile item from `Utilities/Utilities.vcxproj`.
4. Remove vendored `spng.h` project/filter entries.
5. Use package include in `Utilities/PNGHelper.cpp`.

## Validation

- Third-party audit report now excludes libspng vendored component.
- `scripts/audit-third-party-source.ps1 -Enforce` passes with updated policy caps.
- Packaged-mode C++ build path validated for active targets.

## Status

Completed for vendored-source removal and policy/inventory updates.
