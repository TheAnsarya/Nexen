# Issue #2364 ProcessEvent Refactor to Shared Routing Helpers (2026-05-23)

## Scope

- Refactor `Debugger::ProcessEvent` to consume shared routing helpers and reduce inline branch duplication.
- Keep event-specific side effects unchanged.

## Approach

1. Replace inline routed CPU selection with `ResolveEventCpuType`.
2. Replace inline input fallback branch condition with `ShouldFallbackToMainInputDebugger`.
3. Preserve event switch body behavior and existing logs.

## Acceptance Criteria

- `ProcessEvent` contains less branch duplication for routing decisions.
- Routing behavior remains equivalent for all existing event paths.
- Build and targeted tests pass after refactor.
