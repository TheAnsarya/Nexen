# Issue #1299 Root Cause and Fix Plan

## Issue

- GitHub: https://github.com/TheAnsarya/Nexen/issues/1299
- Symptom: breakpoint editor/management path reports "Unsupported cpu type" and manual breakpoint add/edit fails (reported on NES software).

## Root Cause

`BreakpointEditViewModel` builds available memory types by iterating all `MemoryType` enum values and evaluating `cpuType.CanAccessMemoryType(memType)`.

`CpuTypeExtensions.CanAccessMemoryType` falls back to `memType.ToCpuType()` for most memory types.

`MemoryTypeExtensions.ToCpuType` did not map newly introduced Genesis memory types:

- `MemoryType.GenesisMemory`
- `MemoryType.GenesisPrgRom`
- `MemoryType.GenesisWorkRam`
- `MemoryType.GenesisVideoRam`
- `MemoryType.GenesisPaletteRam`

When iteration reached one of those values, `ToCpuType` hit its default case and threw `NotImplementedException("Unsupported cpu type")`, breaking breakpoint UI flows even for non-Genesis consoles.

## Fix Implemented

1. Added Genesis memory-type mappings to `MemoryTypeExtensions.ToCpuType`.
2. Added regression tests in `Tests/Interop/MemoryTypeExtensionsTests.cs`:
	- explicit Genesis mapping checks,
	- full enum coverage check to ensure all concrete memory types map without throwing.

## Validation Plan

- `dotnet test Tests/Nexen.Tests.csproj -c Release --nologo -v m`
- `MSBuild Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m`

## Risk

Low. Changes are mapping/test only; no emulation core behavior changes.

## Follow-Up

- Add CI test coverage gate to ensure newly added `MemoryType` enum values are always mapped by `ToCpuType`.
