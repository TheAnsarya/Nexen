# Epic 18 Plan - Pansy Jump Graph and Flag Fidelity (2026-04-20)

## Context

Nexen Pansy export must preserve complete disassembly metadata for downstream consumers.
This plan aligns with epic #1394 and sub-issues #1395, #1396, #1397, #1398.

## Objectives

- Ensure CODE_DATA_MAP exports standard bits without collisions.
- Keep platform-specific state in dedicated sections (CPU_STATE).
- Export robust CROSS_REFS, including multi-target flows when resolvable.
- Quantify runtime and memory cost of richer graph output.

## Work Breakdown

1. Correct CDM flag mapping and collision handling (#1395).
2. Improve source-to-target edge export for indirect/multi-target cases (#1396).
3. Add benchmark coverage for dense graphs (#1397).
4. Add roundtrip/regression tests with Pansy and Peony (#1398).

## Guardrails

- Accuracy over speed.
- No platform metadata leakage into standard CDM bits.
- Deterministic xref output and stable typing.

## Success Criteria

- Roundtrip tests verify expected flag and edge fidelity.
- Benchmarks show acceptable overhead and publish baseline data.
- Consumers ingest richer metadata without regressions.
