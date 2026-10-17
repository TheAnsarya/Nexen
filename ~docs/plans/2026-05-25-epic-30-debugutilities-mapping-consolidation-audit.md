# Epic 30 DebugUtilities Mapping Consolidation Audit (2026-05-25)

## Goal

Reduce duplicated CpuType/MemoryType mapping logic and centralize debugger mapping policies outside coordinator-heavy callsites.

## Focus Areas

1. Shared CpuType metadata helper for memory mapping and program-counter width.
2. Deterministic regression coverage for mapping compatibility.
3. Refactor callsites to consume consolidated helper outputs.

## Initial Slice Trio

1. #2530 - Add shared CpuType metadata helper.
2. #2531 - Add full mapping regression tests.
3. #2532 - Refactor DebugUtilities callsites to helper mappings.

## Continued Slice Trio

1. #2535 - Extend CpuType metadata helper with PRG-ROM mapping.
2. #2536 - Add PRG-ROM mapping compatibility regression tests.
3. #2537 - Refactor PRG-ROM mapping callsite to metadata helper.

## Progress

- Completed: #2532, #2533, #2534 (shared CpuType metadata for CPU-memory and PC-width mappings).
- Completed: #2535, #2536, #2537 (PRG-ROM mapping consolidation).

## Third Slice Trio

1. #2538 - Add base CPU-memory ownership lookup helper from CpuType metadata.
2. #2541 - Add base CPU-memory ownership mapping regression tests.
3. #2543 - Refactor ToCpuType base-memory path to metadata helper.

## Fourth Slice Trio

1. #2544 - Add shared memory-type predicate/helper for grouped owner mappings.
2. #2545 - Add regression tests for grouped owner mapping parity.
3. #2546 - Refactor DebugUtilities::ToCpuType grouped mappings to helper.

## Fifth Slice Trio

1. #2547 - Add shared constexpr membership helper for memory classification sets.
2. #2548 - Add regression tests for memory classification parity.
3. #2549 - Refactor DebugUtilities classification switches to shared set helper.

## Sixth Slice Trio

1. #2556 - Add shared CPU dispatch metadata helpers for state layout and debugger flags.
2. #2557 - Refactor DebuggerDispatchUtils CPU mapping switches to metadata-driven lookups.
3. #2558 - Add regression tests for DebuggerDispatchUtils mapping parity after helper consolidation.

## Progress

- Completed: #2538, #2541, #2543 (base-memory ownership path consolidation in ToCpuType).
- Completed: #2544, #2545, #2546 (non-relative owner mapping consolidation in ToCpuType).
- Completed: #2547, #2548, #2549 (classification membership helper + switch consolidation).
- In Progress: #2556, #2557, #2558 (dispatch metadata helper + switch consolidation in DebuggerDispatchUtils).

## Acceptance

- Mapping functions preserve behavior.
- Regression tests cover supported CpuType mappings.
- DebugUtilities code has lower duplication and clearer mapping ownership.
