# Platform Parity Benchmark and Correctness Gates

Issue linkage: [#703](https://github.com/TheAnsarya/Nexen/issues/703)

Parent issue: [#673](https://github.com/TheAnsarya/Nexen/issues/673)

## Goal

Define per-phase benchmark and correctness gates for Atari 2600 and Genesis rollout, and provide reusable issue templates for gate-driven implementation tracking.

## Gate Framework

| Gate | Required Evidence | Pass Condition |
|---|---|---|
| Correctness Gate | Deterministic harness summary and targeted tests | All required checks pass, no digest drift |
| Build Gate | Release x64 build output | Build succeeds with no new warnings relevant to changes |
| Performance Gate | Before/after benchmark snapshot | No unexplained regression in tracked metrics |
| Regression Gate | Re-run prior phase checkpoint suite | Previously passing checkpoints remain green |

## Phase Matrix

| Platform Phase | Correctness Harness | Benchmark Focus |
|---|---|---|
| Atari CPU/RIOT skeleton | `Atari2600Bringup*` | Frame-step baseline overhead |
| Atari TIA timing | `Atari2600TiaTiming*` | Scanline/timing checkpoint overhead |
| Atari mapper expansions | Mapper-specific regression suite | Bank-switch dispatch cost |
| Genesis CPU boundary | `GenesisM68k*` | Core stepping overhead |
| Genesis VDP/DMA | `GenesisVdp*` | DMA path and rendering cadence |
| Genesis Z80/audio staging | `GenesisZ80*`, `GenesisAudio*` | Audio pipeline and sync overhead |

## Benchmark Capture Template

```text
Phase:
Baseline Commit:
Candidate Commit:
Benchmark Command:
Key Metrics (before):
Key Metrics (after):
Result: PASS/FAIL
Notes:
```

## Correctness Capture Template

```text
Harness Command:
Checkpoint Summary:
Digest:
Result: PASS/FAIL
Notes:
```

## Roadmap Integration Targets

- [User Future Work Index](../../docs/FUTURE-WORK.md)
- [Q3 Platform Parity Program](platform-parity-research-program-2026-q3.md)
- [Platform Parity Research Index](../research/platform-parity/README.md)

## Issue Template Integration

A reusable GitHub issue template is defined at:

- `.github/ISSUE_TEMPLATE/platform-parity-phase-gate.yml`

Use this template for each new implementation phase to enforce gate evidence and traceability.
