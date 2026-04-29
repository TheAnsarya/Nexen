# Issues #1518, #1520, #1523 - SMS/PBC Readiness and Parity Closeout (2026-04-28)

## Summary

Completed the documentation deliverables for SMS/PBC readiness, UI/runtime parity checklisting, and CI/constraint discoverability.

## Issues Covered

- [#1518](https://github.com/TheAnsarya/Nexen/issues/1518)
- [#1520](https://github.com/TheAnsarya/Nexen/issues/1520)
- [#1523](https://github.com/TheAnsarya/Nexen/issues/1523)

## SMS Readiness Checklist (PBC Assumptions)

| Area | Assumption | Current Evidence | Status |
| ---------- | ---------- | ---------- | ---------- |
| SMS boot path in Genesis host mode | PBC boot flow remains deterministic | `GenesisCompatibilityHarnessTests.PbcHostModeScenarioDigestIsDeterministicAcrossRuns` | Implemented |
| SMS runtime behavior in host mode | Runtime checkpoints are stable | `GEN-COMPAT-PBC-RUNTIME` checkpoint coverage in `GenesisCompatibilityHarnessTests` | Implemented |
| Scenario checkpoint hooking | PBC scenarios expose deterministic hooks | `PbcHostModeScenarioIncludesDeterministicCheckpointHooks` | Implemented |

## UI/Runtime Parity Checklist

| Surface | Expected Parity | Evidence | Gap Status |
| ---------- | ---------- | ---------- | ---------- |
| ROM boot and runtime checkpointing | Same deterministic checkpoint semantics | `GEN-COMPAT-PBC-BOOT`, `GEN-COMPAT-PBC-RUNTIME` | Tracked |
| Shared UI flow discoverability | PBC path discoverable in shared workflows | Existing decomposition chain from #1461 and #1462 | Tracked |
| Runtime diagnostics surfacing | Checkpoint context available for parity work | `PbcRuntimeParityCheckpointContextIsDeterministicAcrossRuns` | Tracked |

## CI and Known-Constraint Documentation

| Item | Description | Status |
| ---------- | ---------- | ---------- |
| CI entry points | `GenesisCompatibilityHarnessTests` PBC scenarios are the baseline CI hook surface | Documented |
| Known constraints | PBC remains under staged compatibility contracts with deterministic checkpoint gates | Documented |
| Discoverability | Linked from [~docs/README.md](../README.md) project tracking table | Documented |

## Remaining Gaps (Issue-Tracked)

- [#1696](https://github.com/TheAnsarya/Nexen/issues/1696) - SMS/PBC implementation backlog after readiness/parity closeout
