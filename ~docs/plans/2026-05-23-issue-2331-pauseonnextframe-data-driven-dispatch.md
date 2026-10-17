# Issue #2331 Plan - PauseOnNextFrame Data-Driven Dispatch

## Scope

Replace `Debugger::PauseOnNextFrame` switch fanout with a compact data-driven CPU-to-scanline mapping helper.

## Acceptance Criteria

- Remove broad switch fanout in `PauseOnNextFrame`.
- Introduce a clear data mapping for supported CPU scanline targets.
- Preserve existing behavior for all currently supported CPU types.
- Keep fallback behavior safe for unsupported CPU types.
- Validate via build and test pass.
