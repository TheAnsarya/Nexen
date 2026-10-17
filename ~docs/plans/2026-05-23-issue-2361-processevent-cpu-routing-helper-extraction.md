# Issue #2361 ProcessEvent CPU Routing Helper Extraction (2026-05-23)

## Scope

- Extract CPU routing decisions from `Debugger::ProcessEvent` into shared `DebuggerDispatchUtils` helpers.
- Keep behavior identical for requested CPU and fallback-to-main routing.

## Approach

1. Add `ResolveEventCpuType(...)` helper in `DebuggerDispatchUtils`.
2. Add focused helper for input-debugger fallback decision.
3. Replace inline routing decisions in `ProcessEvent` with helper usage.

## Acceptance Criteria

- Routing logic is centralized and reusable.
- Existing reroute behavior and logging semantics remain intact.
- No functional regression in event handling.
