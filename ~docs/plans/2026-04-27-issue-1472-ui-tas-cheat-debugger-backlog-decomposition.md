# Issue #1472 - Genesis UI/TAS/Cheat/Debugger Backlog Decomposition (2026-04-27)

## Summary

Decomposed the Genesis-family full UX/tooling backlog into focused tracks for configuration UX parity, TAS/cheat feature-surface parity with deterministic test planning, and debugger/register/event parity plus documentation discoverability.

## Parent

- [#1472](https://github.com/TheAnsarya/Nexen/issues/1472)

## Child Execution Slices

- [#1548](https://github.com/TheAnsarya/Nexen/issues/1548) - [24.5.1] Genesis-family configuration UX parity backlog matrix
- [#1549](https://github.com/TheAnsarya/Nexen/issues/1549) - [24.5.2] TAS/cheat feature-surface parity backlog and deterministic test plan
- [#1550](https://github.com/TheAnsarya/Nexen/issues/1550) - [24.5.3] Debugger/register/event tooling parity and documentation link-tree closure

## Decomposition Rationale

Issue #1472 combines three large backlog domains with distinct ownership and validation needs:

- Configuration UX parity and discoverability
- TAS/cheat parity with determinism-focused coverage planning
- Debugger/tooling parity plus docs/link-tree closure requirements

Splitting by domain gives clearer prioritization, measurable gates, and incremental closure evidence.

## Baseline Validation Snapshot

Focused runtime suites remain green in current branch state:

- `GenesisRuntimeTranscriptHandshakeTests.*`
- `GenesisSegaCdTranscriptLaneTests.*`
- `GenesisSegaCdProtocolCadenceValidationTests.*`

Current sequence baseline: 25/25 passing.

## Execution Order

1. #1548
2. #1549
3. #1550

Close #1472 after child-track backlog matrices, test-plan linkage, and docs discoverability evidence are established.
