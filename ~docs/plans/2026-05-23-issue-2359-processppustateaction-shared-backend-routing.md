# Issue #2359 ProcessPpuStateAction Shared Backend Routing (2026-05-23)

## Scope

- Refactor `Debugger::ProcessPpuStateAction` to use shared PPU backend mapping enum.
- Reduce duplicated route-decision logic and keep per-backend action dispatch behavior unchanged.

## Approach

1. Update `ProcessPpuStateAction` to switch on `GetPpuStateBackendForCpu(cpuType)`.
2. Keep backend-to-debugger dispatch targets unchanged.
3. Preserve current no-op behavior for unsupported backends.

## Acceptance Criteria

- Routing decision source is shared utility mapping.
- Dispatch to each backend debugger remains identical.
- Build and targeted utility tests succeed after refactor.
