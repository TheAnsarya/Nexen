# Issue #2326 Plan - SaveRom Polymorphic Dispatch Unification

## Scope

Refactor `Debugger::SaveRomToDisk` to use `IDebugger` polymorphism instead of the current broad per-CPU switch fanout, while preserving the SGB routing behavior.

## Acceptance Criteria

- Add virtual `SaveRomToDisk(...)` API to `IDebugger` with safe default behavior.
- Mark platform-specific debugger `SaveRomToDisk` methods as overrides.
- Replace central switch in `Debugger::SaveRomToDisk` with polymorphic dispatch.
- Preserve SGB routing semantics (SNES + active GB debugger path).
- Validate with focused ThemeProfile tests and Release x64 build.
