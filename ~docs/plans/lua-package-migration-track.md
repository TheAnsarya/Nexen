# Lua Package Migration Track

Issue: #2187
Parent: #2185

## Goal

Remove vendored `Lua/*` source from Nexen and consume Lua through package-managed dependencies while preserving Nexen-specific behavior.

## Constraints

- Preserve all existing scripting behavior and debugger integrations.
- Preserve Nexen watchdog and safety semantics currently embedded in vendored Lua.
- Keep C++ tests passing and avoid runtime behavior regressions.

## Current State

- Vendored Lua remains the largest tracked third-party source footprint.
- Inventory baseline (from [../../docs/THIRD-PARTY-SOURCE-INVENTORY.md](../../docs/THIRD-PARTY-SOURCE-INVENTORY.md)):
	- Lua files: 103
	- Lua lines: 31818
	- Lua bytes: 1113623

## Plan

1. Identify all Nexen-specific Lua deltas versus upstream/package Lua.
2. Implement a patch layer isolated from vendor sources (wrapper hooks/adapter headers).
3. Switch build targets from `Lua/Lua.vcxproj` source compilation to package-linked Lua.
4. Validate scripting, debugger callbacks, and serialization behavior.
5. Remove vendored Lua source and update third-party inventory/policy baselines.

## Validation

- Build: Release x64
- Tests:
	- `Core.Tests.exe --gtest_filter=Lua*`
	- `Core.Tests.exe --gtest_filter=Debugger*`
	- Existing Genesis/targeted suites unaffected

## Deliverables

- Build-system switch to package dependency.
- Patch-layer docs + code.
- Updated inventory showing Lua footprint reduction.
