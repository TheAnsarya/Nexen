# Channel F Architecture

## Purpose

This document describes how Fairchild Channel F support is organized in Nexen across core emulation, UI integration, debugger surfaces, TAS/movie workflows, and persistence.

## High-Level Architecture

```text
ROM Loader
    -> Channel F Core Wiring
        -> CPU/Timing/Video/Audio/Input
            -> Shared Nexen Services
                -> Save States
                -> Movie and TAS
                -> Debugger
                -> UI Windows
```

Channel F follows the same cross-system architecture pattern used by other first-class platforms in Nexen: the platform-specific core feeds standardized emulator services, and those services drive shared UI and tooling layers.

## Core Emulation Layer

The core layer is responsible for deterministic execution, device timing, memory/register behavior, and per-frame state updates.

Primary responsibilities:

- Instruction execution and timing progression.
- Video state updates and frame output handoff.
- Input polling and normalization into shared controller state.
- Audio production through the common mixer path.
- Snapshot-friendly state layout for save-state determinism.

## UI and User Workflow Integration

Channel F uses Nexen's existing user-facing flow:

1. Open ROM from the standard file workflow.
2. Run gameplay with optional pause, frame-step, rewind, and save states.
3. Record movies for deterministic playback and analysis.
4. Open TAS editor for frame-by-frame edits.
5. Open debugger windows for instruction and memory investigation.

This avoids one-off platform UX paths and keeps Channel F discoverable through existing menus and dialogs.

## Debugger Integration

Channel F debugger support follows the shared debugger model:

- Disassembly view for instruction flow and breakpoint placement.
- Memory view for register and memory inspection.
- Trace logging for ordered execution analysis.
- Event/log surfaces to correlate runtime activity.

Quality goals:

- Breakpoint and step behavior must be deterministic.
- Memory and trace views must agree for inspected write and read paths.
- Debugger interactions must remain stable while TAS/movie workflows are active.

See also: [Debugging Guide](Debugging.md)

## TAS and Movie Integration

Channel F TAS and movie support uses the same shared subsystems as other platforms:

- Movie recording and playback for deterministic runs.
- TAS editor frame-grid editing, route branching, and replay loops.
- Save-state-assisted iterative workflows.
- Import and export compatibility through movie tooling where applicable.

Quality goals:

- Input mapping remains consistent between live gameplay, movie data, and TAS UI.
- Branching and rerecord workflows preserve deterministic replay.
- Save/load interactions do not corrupt movie or TAS timelines.

See also:

- [Movie System Guide](Movie-System.md)
- [TAS Editor Manual](TAS-Editor-Manual.md)

## Persistence and Determinism

Persistence coverage for Channel F includes:

- Save-state serialization and restore consistency.
- Movie file roundtrip consistency.
- Runtime configuration persistence in shared settings paths.

Determinism expectations:

- Replaying identical movie inputs under equivalent settings reproduces equivalent behavior.
- Save-state restore points are stable for debugger and TAS handoff workflows.

## Validation Path

Use these companion documents for repeatable verification:

- [Channel F Debug and TAS Manual Test](manual-testing/channelf-debug-and-tas-manual-test.md)
- [Channel F Test Strategy Matrix](../~docs/testing/channelf-test-strategy-matrix-2026-03-30.md)
