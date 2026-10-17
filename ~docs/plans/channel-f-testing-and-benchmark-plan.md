# Channel F Testing and Benchmark Plan

## Testing Strategy

## 1. Unit Tests

- F8 opcode semantics by instruction family.
- Bus/memory mapping edge cases.
- Input decoding and panel event behavior.
- Cartridge RAM persistence behavior.

## 2. Integration Tests

- BIOS boot and built-in game smoke tests.
- Representative cartridge launch matrix.
- Save/load determinism checks.
- UI open/configure/run workflows.

## 3. Differential Accuracy Tests

- Frame hash comparisons versus reference captures.
- Instruction trace comparison windows for selected ROM scenarios.
- Input-response timing checkpoints for known control-sensitive games.

## 4. Tooling Tests

- Debugger stepping and breakpoint correctness.
- Disassembler opcode textual validation.
- Pansy export/import roundtrip consistency.
- Lua scripting API behavior checks.

## 5. TAS/Movie Tests

- Record/playback determinism.
- Re-record branch integrity.
- Input encoding coverage for twist/pull/push/panel actions.

## Benchmark Plan

## Core Benchmarks

- Opcode dispatch throughput.
- Frame emulation time (CPU+video+audio combined).
- Video scanline renderer cost.
- Audio generation overhead.

## Tooling Benchmarks

- Debugger attached vs detached overhead.
- Trace logger cost per frame.
- Pansy export generation throughput.

## Memory Benchmarks

- Peak memory by ROM/profile.
- Allocation count per second in steady-state gameplay.
- Save-state memory footprint.

## Pass/Fail Gates

- Accuracy gate: no unexplained diff in approved ROM matrix.
- Perf gate: benchmark regressions beyond threshold fail CI lane.
- Memory gate: allocation and peak memory budgets enforced.
- Determinism gate: replay hash must match baseline.

## Reporting

- Store benchmark artifacts in standardized JSON and markdown summaries.
- Post issue comments with before/after evidence per optimization change.
- Keep session logs synchronized with gate status progression.
