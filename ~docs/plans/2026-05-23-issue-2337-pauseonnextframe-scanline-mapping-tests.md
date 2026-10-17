# Issue #2337 PauseOnNextFrame Scanline Mapping Tests (2026-05-23)

## Scope

- Add direct regression coverage for PauseOnNextFrame scanline mapping behavior.
- Improve testability by extracting mapping logic into a shared utility.
- Verify mapped and unmapped CPU behavior with deterministic unit tests.

## Approach

1. Move PauseOnNextFrame CPU-to-scanline mapping into `DebuggerDispatchUtils.h`.
2. Update `Debugger.cpp` to consume shared utility function.
3. Add `Core.Tests` coverage for known mapped values and unsupported CPU fallback behavior.

## Acceptance Criteria

- New tests validate all mapped systems and unsupported fallback paths.
- `Debugger.cpp` no longer owns duplicate private mapping table.
- Targeted `Core.Tests` filter run passes.
