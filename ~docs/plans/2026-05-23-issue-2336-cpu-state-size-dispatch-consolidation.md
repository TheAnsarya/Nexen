# Issue #2336 CPU State Size Dispatch Consolidation (2026-05-23)

## Scope

- Remove duplicated `CpuType` switch logic used by both `Debugger::GetCpuState` and `Debugger::SetCpuState`.
- Preserve exact memcpy size behavior for every currently supported CPU type.
- Keep implementation correctness-safe with no emulation-behavior change.

## Approach

1. Add a centralized helper in `Debugger.cpp` that maps `CpuType` to state-struct byte size.
2. Replace duplicated state-copy switches with shared helper-backed memcpy calls.
3. Keep state-ref acquisition and break-helper semantics unchanged.

## Acceptance Criteria

- `GetCpuState` and `SetCpuState` use a single source of truth for copy-size mapping.
- Existing per-CPU state-size behavior is preserved, including shared SNES/SA1 size mapping.
- Release x64 build succeeds.
