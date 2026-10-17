# Issue #2351 BreakSource Config Mapping in Shared Dispatch Utils (2026-05-23)

## Scope

- Centralize BreakSource-to-DebugConfig option mapping in shared debugger dispatch utilities.
- Preserve existing behavior for mapped sources and default-true behavior for unmapped/internal/user break sources.

## Approach

1. Add `IsBreakOptionEnabledForSource(BreakSource, const DebugConfig&)` in `DebuggerDispatchUtils`.
2. Refactor `Debugger::IsBreakOptionEnabled` to delegate to the shared utility helper.
3. Keep mapping keys and semantics exactly aligned with existing DebugConfig options.

## Acceptance Criteria

- BreakSource mapping logic is defined in one utility function.
- `Debugger::IsBreakOptionEnabled` is reduced to simple delegation.
- Runtime behavior remains unchanged for all mapped and unmapped sources.
