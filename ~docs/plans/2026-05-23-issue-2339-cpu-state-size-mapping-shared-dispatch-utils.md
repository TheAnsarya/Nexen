# Issue #2339 CPU State-Size Mapping in Shared Dispatch Utils (2026-05-23)

## Scope

- Move CPU family/layout mapping used for debugger state-copy sizing into shared dispatch utilities.
- Keep byte-size (`sizeof`) ownership in `Debugger.cpp` while centralizing CPU-to-layout decision logic.
- Preserve all existing CPU mapping behavior exactly.

## Approach

1. Add `CpuStateLayout` enum in `DebuggerDispatchUtils`.
2. Add `GetCpuStateLayout(CpuType)` shared utility mapping for all CPU families.
3. Update `Debugger.cpp` CPU-size helper to switch on shared `CpuStateLayout` values.

## Acceptance Criteria

- `Debugger.cpp` no longer duplicates CPU-family mapping logic for state-size selection.
- Existing runtime behavior and mapped sizes remain unchanged.
- Release x64 build succeeds.
