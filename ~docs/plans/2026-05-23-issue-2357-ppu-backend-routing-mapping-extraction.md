# Issue #2357 PPU Backend Routing Mapping Extraction (2026-05-23)

## Scope

- Extract CPU-to-PPU-backend routing mapping into shared debugger dispatch utilities.
- Preserve existing grouped SNES coprocessor routing semantics.

## Approach

1. Add `PpuStateBackend` enum to `DebuggerDispatchUtils`.
2. Add `GetPpuStateBackendForCpu(CpuType)` utility mapping.
3. Keep mapping behavior unchanged for all supported CPU types.

## Acceptance Criteria

- PPU backend selection logic is available via shared utility function.
- SNES/SA1/SPC/GSU/Cx4/ST018 continue mapping to SNES PPU backend.
- Unsupported CPU types map to explicit `None` backend.
