# Genesis M68000 Integration Boundary and Bring-Up Spike

Issue linkage: [#700](https://github.com/TheAnsarya/Nexen/issues/700)

Parent issue: [#696](https://github.com/TheAnsarya/Nexen/issues/696)

## Goal

Define the minimum stable M68000 integration boundary for Nexen and a phased bring-up scaffold that compiles cleanly.

## Scope

- CPU execution contract and interface methods.
- Memory-map ownership boundaries across CPU, VDP, and bus arbitration layer.
- Interrupt and exception touchpoints.
- Build-time scaffolding for phased implementation.

## CPU Contract Draft

| Method | Purpose |
|---|---|
| `Reset()` | Initialize CPU and internal registers |
| `StepCycles(n)` | Advance execution with visible cycle accounting |
| `SetInterrupt(level)` | Apply interrupt line changes from VDP/bus sources |
| `AttachBus(bus)` | Bind CPU memory operations to platform bus layer |

## Memory Ownership Boundary Draft

- CPU reads/writes through a single bus abstraction.
- Bus layer owns routing to ROM, RAM, VDP registers, Z80 windows, and I/O.
- CPU core does not own platform-specific decode logic.

## Bring-Up Sequence

1. Add CPU interface and stub implementation behind platform feature flag.
2. Add bus interface with placeholder decode endpoints.
3. Wire interrupt entry points without full device behavior.
4. Add compile-only scaffolding test and smoke startup path.

## Exit Criteria

- Architecture notes define stable interfaces and extension points.
- Build-time scaffolding compiles in Release x64.
- Follow-up implementation tasks are listed and dependency-ordered.

## Deferred Future-Work Linkage

Status: Future Work only. Do not start these issues until explicitly scheduled.

- Parent future-work epic: [#718](https://github.com/TheAnsarya/Nexen/issues/718)
- M68000 semantics follow-through: [#728](https://github.com/TheAnsarya/Nexen/issues/728)
- Memory-map and bus ownership follow-through: [#729](https://github.com/TheAnsarya/Nexen/issues/729)
- Interrupt/frame event scheduling follow-through: [#733](https://github.com/TheAnsarya/Nexen/issues/733)
- Z80 bus handoff follow-through: [#734](https://github.com/TheAnsarya/Nexen/issues/734)

## Execution Evidence: Promoted M68000 Semantics Follow-Through

Status: Completed from deferred backlog (2026-03-17)

### Issue [#728](https://github.com/TheAnsarya/Nexen/issues/728)

- Replaced per-cycle PC bump stub behavior with deterministic instruction cadence in `GenesisM68kCpuStub::StepCycles`.
- Added interrupt/exception entry flow with vector fetch, supervisor stack pushes, SR interrupt mask latch, and deterministic service-cycle window.
- Extended CPU scaffold state exposure for deterministic checkpoint assertions (`GetStatusRegister`, `GetSupervisorStackPointer`, `GetLastExceptionVectorAddress`, `GetInterruptSequenceCount`).
- Expanded focused tests in `GenesisM68kBoundaryScaffoldTests` to validate:
	- deterministic instruction stepping cadence,
	- interrupt vector dispatch,
	- exception-state latching and stack pointer progression.

Validation commands:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Genesis* --gtest_brief=1
.\bin\win-x64\Release\Core.Tests.exe --gtest_brief=1
dotnet test --no-build -c Release
```

Result: 9 Genesis tests passed; 1682 native tests passed; 331 managed tests passed.

## Related Research

- [Genesis M68000 CPU Integration](../research/platform-parity/genesis/cpu-m68000.md)
- [Genesis Architecture Overview](../research/platform-parity/genesis/architecture-overview.md)
