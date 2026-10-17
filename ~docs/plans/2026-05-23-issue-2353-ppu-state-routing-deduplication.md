# Issue #2353 PPU State Routing Deduplication (2026-05-23)

## Scope

- Reduce duplication between `Debugger::GetPpuState` and `Debugger::SetPpuState` CPU routing logic.
- Preserve routing behavior across all currently supported PPU-debugger backends.

## Approach

1. Introduce shared member template helper to route PPU state actions by CPU type.
2. Refactor both get and set methods to call shared helper with operation lambdas.
3. Keep DebugBreakHelper behavior and routing targets unchanged.

## Acceptance Criteria

- CPU routing switch is defined once for PPU state operations.
- `GetPpuState` and `SetPpuState` remain behaviorally identical.
- Release build and targeted tests succeed after refactor.
