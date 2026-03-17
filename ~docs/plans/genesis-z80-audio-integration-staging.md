# Genesis Z80 and Audio Subsystem Integration Staging

Issue linkage: [#702](https://github.com/TheAnsarya/Nexen/issues/702)

Parent issue: [#696](https://github.com/TheAnsarya/Nexen/issues/696)

## Goal

Create a staged integration plan for Z80, YM2612, and SN76489 that preserves correctness while keeping each phase independently testable.

## Scope

- Z80 ownership boundaries and bus handshake touchpoints.
- YM2612 and SN76489 phased activation model.
- Deterministic staged validation checkpoints.

## Integration Phases

| Phase | Focus | Validation Surface |
|---|---|---|
| P1 | Z80 bootstrap and bus ownership arbitration | Z80-BOOT-* |
| P2 | SN76489 baseline routing and timing coherence | PSG-BASE-* |
| P3 | YM2612 baseline synthesis and register sequencing | FM-BASE-* |
| P4 | Mixed-audio execution with synchronization checks | AUDIO-SYNC-* |

## Synchronization Boundaries

| Boundary | Producer | Consumer | Assertion |
|---|---|---|---|
| BUS-ARB-01 | 68000 bus layer | Z80 execution loop | Ownership transitions are ordered and deterministic |
| AUD-CLOCK-01 | Master scheduler | PSG/FM generators | Clock deltas remain within expected phase windows |
| AUD-MIX-01 | PSG/FM output stages | Mixer/output buffer | Frame-aligned sample windows remain stable across replays |

## Audio Regression Checks Per Phase

| Phase | Check ID | Pass Condition |
|---|---|---|
| P1 | Z80-BOOT-01 | Z80 bootstrap and bus arbitration checkpoints pass |
| P2 | PSG-BASE-01 | PSG output digest stable across repeated runs |
| P3 | FM-BASE-01 | FM register sequencing digest stable across repeated runs |
| P4 | AUDIO-SYNC-01 | Mixed output digest and sync assertions remain stable |
| P4 | AUDIO-LAT-01 | End-to-end audio latency remains within defined phase budget |

## Risk Register

| Risk | Impact | Mitigation |
|---|---|---|
| Bus arbitration mismatch | CPU/Z80 desync | Add explicit contention traces and invariant checks |
| Audio clock drift | Audible instability and replay mismatch | Introduce periodic drift assertions and digest checks |
| Partial integration regressions | Hidden breakage between phases | Keep each phase behind independent tests and toggles |

## Validation Command Templates

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=GenesisZ80* --gtest_brief=1
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=GenesisAudio* --gtest_brief=1
```

## Exit Criteria

- Integration phases are documented with clear dependencies.
- Risk register maps directly to planned validation checkpoints.
- A staged rollout path exists that avoids all-at-once subsystem landing.
- Audio determinism and latency checks are documented per phase.

## Execution Evidence: Promoted Z80 Bootstrap and Bus Handoff Follow-Through

Status: Completed from deferred backlog (2026-03-17)

### Issue [#734](https://github.com/TheAnsarya/Nexen/issues/734)

- Added deterministic Z80 bootstrap/run state and bus-request handoff controls in `GenesisPlatformBusStub`.
- Added deterministic shared-bus arbitration behavior for the Z80 window when Z80 is running versus bus-request handoff active.
- Added deterministic Z80 cycle progression tracking through scaffold stepping (`StepZ80Cycles`).
- Added focused `GenesisZ80HandoffTests` to validate:
	- bootstrap state transition behavior,
	- bus-request halt/resume handoff behavior,
	- Z80 window access behavior under ownership handoff.

Validation commands:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Genesis* --gtest_brief=1
.\bin\win-x64\Release\Core.Tests.exe --gtest_brief=1
dotnet test --no-build -c Release
```

Result: 28 Genesis tests passed; 1701 native tests passed; 331 managed tests passed.

## Related Research

- [Genesis Z80 Audio Bus](../research/platform-parity/genesis/z80-audio-bus.md)
- [Genesis YM2612 and SN76489](../research/platform-parity/genesis/ym2612-sn76489-audio.md)
