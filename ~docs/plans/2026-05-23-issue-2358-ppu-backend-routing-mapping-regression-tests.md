# Issue #2358 PPU Backend Routing Mapping Regression Tests (2026-05-23)

## Scope

- Add direct unit tests for shared PPU backend routing mapping utility.
- Verify both supported and unsupported CPU routing outcomes.

## Approach

1. Extend `DebuggerDispatchUtilsTests` with assertions for all routed CPU types.
2. Verify SNES-family grouping and per-system mapping targets.
3. Add unsupported mapping assertion (`Genesis` => `None`).

## Acceptance Criteria

- Utility tests cover the complete PPU routing map.
- Grouped SNES-family routing is explicitly locked by tests.
- Unsupported fallback behavior is covered and passing.
