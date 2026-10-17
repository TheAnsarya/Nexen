# Issue #2341 Debug Window Flag Mapping in Shared Dispatch Utils (2026-05-23)

## Scope

- Consolidate `IsDebugWindowOpened` CPU-to-flag mapping into shared dispatch utilities.
- Keep unsupported CPU behavior explicit (no mapped flag => false).

## Approach

1. Add `GetDebuggerFlagForCpu(CpuType)` to `DebuggerDispatchUtils` returning optional flag.
2. Refactor `Debugger::IsDebugWindowOpened` to consume shared optional mapping.
3. Add regression tests for mapped CPU flags and unsupported fallback behavior.

## Acceptance Criteria

- `IsDebugWindowOpened` no longer contains a large per-CPU switch.
- Mapped CPUs resolve to expected `DebuggerFlags` values.
- Unmapped CPU types (for example Genesis today) remain false/no-flag behavior.
